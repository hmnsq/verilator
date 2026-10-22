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

class Ast_ग्रन्थिः_Cons_Item VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    friend class Ast_ग्रन्थिः_व्यञ्जकम्;  // For m_knownConstantTree
    mutable bool m_known_Constant_वृक्षः = false;  // Known to be a tree of constants

रक्षित:
    Ast_ग्रन्थिः_Cons_Item(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : Ast_ग्रन्थिः{t, fl} {
        this->valuep(valuep);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Cons_Item;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class Ast_ग्रन्थिः_Cover_Decl VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    पूर्णाङ्क m_bin_अङ्कः = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    पूर्णाङ्क m_local_Bin_अङ्कः = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
सार्वजनिक:
    Ast_ग्रन्थिः_Cover_Decl(VNType t, File_पङ्क्तिः* fl, ध्रुव string& page, ध्रुव string& comment)
        : Ast_ग्रन्थिः(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Cover_Decl;
    ध्रुव वर्ण* broken() ध्रुव override {
        सचे (m_dataDeclp
            && (m_dataDeclp == this || m_dataDeclp->m_dataDeclp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        पटिनिवत्त nullptr;
    }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { पटिनिवत्त 1 + 2 * INSTR_गणनम्_LD; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    पूर्णाङ्क bin_अङ्कः() ध्रुव { पटिनिवत्त m_bin_अङ्कः; }
    शून्य bin_अङ्कः(पूर्णाङ्क flag) { m_bin_अङ्कः = flag; }
    पूर्णाङ्क local_Bin_अङ्कः() ध्रुव { पटिनिवत्त m_local_Bin_अङ्कः; }
    शून्य local_Bin_अङ्कः(पूर्णाङ्क flag) { m_local_Bin_अङ्कः = flag; }
    bool perInstance() ध्रुव { पटिनिवत्त m_perInstance; }
    शून्य perInstance(bool flag) { m_perInstance = flag; }
    आभास पूर्णाङ्क size() ध्रुव = 0;
    ध्रुव string& comment() ध्रुव { पटिनिवत्त m_text; }  // text to insert in code
    ध्रुव string& page() ध्रुव { पटिनिवत्त m_page; }
    ध्रुव string& hier() ध्रुव { पटिनिवत्त m_hier; }
    शून्य hier(ध्रुव string& flag) { m_hier = flag; }
    शून्य comment(ध्रुव string& flag) { m_text = flag; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव Ast_ग्रन्थिः_Cover_Decl* ध्रुव asamep = VN_DBG_AS(samep, ग्रन्थिः_Cover_Decl);
        पटिनिवत्त (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() ध्रुव override { पटिनिवत्त false; }
    शून्य dataDeclp(Ast_ग्रन्थिः_Cover_Decl* nodep) { m_dataDeclp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_ग्रन्थिः_Cover_Decl* dataDeclNullp() ध्रुव { पटिनिवत्त m_dataDeclp; }
    Ast_ग्रन्थिः_Cover_Decl* dataDeclThisp() { पटिनिवत्त dataDeclNullp() ? dataDeclNullp() : this; }
};
class Ast_ग्रन्थिः_FTask VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
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
    Ast_ग्रन्थिः_FTask(VNType t, File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* stmtsp)
        : Ast_ग्रन्थिः{t, fl}
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
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_FTask;
    आभास Ast_ग्रन्थिः_FTask* cloneType(ध्रुव string& name) = 0;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Var name
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    bool isGateOptimizable() ध्रुव override {
        पटिनिवत्त !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string cname() ध्रुव { पटिनिवत्त m_cname; }
    शून्य cname(ध्रुव string& cname) { m_cname = cname; }
    bool isFunction() ध्रुव { पटिनिवत्त fvarp() != nullptr; }
    // MORE ACCESSORS
    शून्य dpiOpenParentInc() { ++m_dpiOpenParent; }
    शून्य dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() ध्रुव { पटिनिवत्त m_dpiOpenParent; }
    bool taskPublic() ध्रुव { पटिनिवत्त m_taskPublic; }
    शून्य taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() ध्रुव { पटिनिवत्त m_classMethod; }
    शून्य classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() ध्रुव { पटिनिवत्त m_didProto; }
    शून्य didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ध्रुव { पटिनिवत्त m_isExternProto; }
    शून्य isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ध्रुव { पटिनिवत्त m_isExternDef; }
    शून्य isExternDef(bool flag) { m_isExternDef = flag; }
    ध्रुव string& ifacePortName() ध्रुव { पटिनिवत्त m_ifacePortName; }
    शून्य ifacePortName(ध्रुव string& name) { m_ifacePortName = name; }
    bool prototype() ध्रुव { पटिनिवत्त m_prototype; }
    शून्य prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ध्रुव { पटिनिवत्त m_dpiExport; }
    शून्य dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ध्रुव { पटिनिवत्त m_dpiImport; }
    शून्य dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ध्रुव { पटिनिवत्त m_dpiContext; }
    शून्य dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() ध्रुव { पटिनिवत्त m_dpiOpenChild; }
    शून्य dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() ध्रुव { पटिनिवत्त m_dpiTask; }
    शून्य dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ध्रुव { पटिनिवत्त !m_dpiCDecl.empty(); }
    ध्रुव string& dpiCDecl() ध्रुव { पटिनिवत्त m_dpiCDecl; }
    शून्य dpiCDecl(ध्रुव string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ध्रुव { पटिनिवत्त m_isConstructor; }
    शून्य isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ध्रुव { पटिनिवत्त m_isHideLocal; }
    शून्य isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ध्रुव { पटिनिवत्त m_isHideProtected; }
    शून्य isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() ध्रुव { पटिनिवत्त m_dpiPure; }
    शून्य dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() ध्रुव { पटिनिवत्त m_keepAlive; }
    शून्य keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ध्रुव { पटिनिवत्त m_needsSyms; }
    शून्य needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() ध्रुव { पटिनिवत्त m_pureVirtual; }
    शून्य pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() ध्रुव { पटिनिवत्त m_recursive; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ध्रुव { पटिनिवत्त m_static; }
    शून्य isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ध्रुव { पटिनिवत्त m_underGenerate; }
    शून्य underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ध्रुव { पटिनिवत्त m_verilogFunction; }
    शून्य verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ध्रुव { पटिनिवत्त m_verilogTask; }
    शून्य verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ध्रुव { पटिनिवत्त m_virtual; }
    शून्य isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ध्रुव { पटिनिवत्त m_needProcess; }
    शून्य setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() ध्रुव { पटिनिवत्त m_isCovergroupSample; }
    शून्य isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    शून्य baseOverride(ध्रुव VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ध्रुव { पटिनिवत्त m_baseOverride; }
    शून्य lifetime(ध्रुव VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ध्रुव { पटिनिवत्त m_lifetime; }
    bool isPure() override;
    ध्रुव वर्ण* broken() ध्रुव override;
    शून्य propagateAttrFrom(ध्रुव Ast_ग्रन्थिः_FTask* fromp) {
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
    bool getPurityRecurse() ध्रुव;
};
class Ast_ग्रन्थिः_File VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
सार्वजनिक:
    Ast_ग्रन्थिः_File(VNType t, File_पङ्क्तिः* fl, ध्रुव string& name)
        : Ast_ग्रन्थिः{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_File;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { पटिनिवत्त true; }
};
class Ast_ग्रन्थिः_Func_Cov_Item VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // Base class for functional coverage items (coverpoints, crosses)
रक्षित:
    string m_name;  // Item name

सार्वजनिक:
    Ast_ग्रन्थिः_Func_Cov_Item(VNType t, File_पङ्क्तिः* fl, ध्रुव string& name)
        : Ast_ग्रन्थिः{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Func_Cov_Item;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    bool maybePointedTo() ध्रुव override { पटिनिवत्त true; }
};
class Ast_ग्रन्थिः_Gen VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // Generate construct
सार्वजनिक:
    Ast_ग्रन्थिः_Gen(VNType t, File_पङ्क्तिः* fl)
        : Ast_ग्रन्थिः{t, fl} {}
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Gen;
};
class Ast_ग्रन्थिः_Module VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    ध्रुव string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
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
    Ast_ग्रन्थिः_Module(VNType t, File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : Ast_ग्रन्थिः{t, fl}
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
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Module;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    आभास bool timescaleMatters() ध्रुव = 0;
    अन्तःस्थ bool isDollarUnit() ध्रुव;  // Is the $unit package
    अन्तःस्थ bool isConstPool() ध्रुव;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ध्रुव string& outer) ध्रुव;
    // ACCESSORS
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string origName() ध्रुव override { पटिनिवत्त m_origName; }
    string someInstanceName() ध्रुव VL_MT_SAFE { पटिनिवत्त m_someInstanceName; }
    शून्य someInstanceName(ध्रुव string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ध्रुव { पटिनिवत्त m_defaultDisablep; }
    शून्य defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() ध्रुव { पटिनिवत्त m_inLibrary; }
    शून्य inLibrary(bool flag) { m_inLibrary = flag; }
    शून्य depth(पूर्णाङ्क मानम्) { m_depth = मानम्; }
    पूर्णाङ्क depth() ध्रुव VL_MT_SAFE { पटिनिवत्त m_depth; }
    शून्य level(पूर्णाङ्क मानम्) { m_level = मानम्; }
    पूर्णाङ्क level() ध्रुव VL_MT_SAFE { पटिनिवत्त m_level; }
    string libname() ध्रुव { पटिनिवत्त m_libname; }
    bool is_शिखरम्() ध्रुव VL_MT_SAFE { पटिनिवत्त level() == 1; }
    bool modPublic() ध्रुव { पटिनिवत्त m_modPublic; }
    शून्य modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() ध्रुव { पटिनिवत्त m_modTrace; }
    शून्य modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ध्रुव { पटिनिवत्त m_ctorVarReset; }
    शून्य ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ध्रुव { पटिनिवत्त m_dead; }
    शून्य dead(bool flag) { m_dead = flag; }
    bool hasGParam() ध्रुव { पटिनिवत्त m_hasGParam; }
    शून्य hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ध्रुव { पटिनिवत्त m_hasParameterList; }
    शून्य hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() ध्रुव { पटिनिवत्त m_hierBlock; }
    शून्य hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() ध्रुव { पटिनिवत्त m_hierParams; }
    शून्य hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ध्रुव { पटिनिवत्त m_recursive; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    शून्य recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ध्रुव { पटिनिवत्त m_recursiveClone; }
    bool parameterizedTemplate() ध्रुव { पटिनिवत्त m_parameterizedTemplate; }
    शून्य parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    शून्य verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ध्रुव { पटिनिवत्त m_verilatorLib; }
    VLifetime lifetime() ध्रुव { पटिनिवत्त m_lifetime; }
    शून्य lifetime(ध्रुव VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ध्रुव { पटिनिवत्त m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ध्रुव { पटिनिवत्त m_unconnectedDrive; }
    शून्य unconnectedDrive(ध्रुव VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_ग्रन्थिः_Procedure VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
रक्षित:
    Ast_ग्रन्थिः_Procedure(VNType t, File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : Ast_ग्रन्थिः{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Procedure;
    // METHODS
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isJustOneBodyStmt() ध्रुव { पटिनिवत्त stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ध्रुव { पटिनिवत्त m_suspendable; }
    शून्य setSuspendable() { m_suspendable = true; }
    bool needProcess() ध्रुव { पटिनिवत्त m_needProcess; }
    शून्य setNeedProcess() { m_needProcess = true; }
};
class Ast_ग्रन्थिः_Range VL_NOT_FINAL : सार्वजनिक Ast_ग्रन्थिः {
    // A range, sized or unsized
रक्षित:
    Ast_ग्रन्थिः_Range(VNType t, File_पङ्क्तिः* fl)
        : Ast_ग्रन्थिः{t, fl} {}

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_ग्रन्थिः_Range;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : सार्वजनिक Ast_ग्रन्थिः {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

सार्वजनिक:
    AstActive(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_Sen_वृक्षः* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    // Statements are broken into pieces, as some must come before others.
    शून्य sentreep(Ast_Sen_वृक्षः* nodep) { m_sentreep = nodep; }
    Ast_Sen_वृक्षः* sentreep() ध्रुव { पटिनिवत्त m_sentreep; }
    // METHODS
    अन्तःस्थ bool hasClocked() ध्रुव;
    अन्तःस्थ bool hasCombo() ध्रुव;
    अन्तःस्थ bool hasInitial() ध्रुव;
    अन्तःस्थ bool hasStatic() ध्रुव;
};
class AstAlias final : सार्वजनिक Ast_ग्रन्थिः {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
सार्वजनिक:
    AstAlias(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : सार्वजनिक Ast_ग्रन्थिः {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
सार्वजनिक:
    AstAliasScope(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
सार्वजनिक:
    AstBind(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Bind Target name
    शून्य name(ध्रुव string& name) override { m_name = name; }
};
class AstCFunc final : सार्वजनिक Ast_ग्रन्थिः {
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
    AstCFunc(File_पङ्क्तिः* fl, ध्रुव string& name, AstScope* scopep, ध्रुव string& rtnType = "")
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
        m_cost = v3Global.opt.instr_गणनम्_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstCFunc* ध्रुव asamep = VN_DBG_AS(samep, CFunc);
        पटिनिवत्त ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    शून्य name(ध्रुव string& name) override { m_name = name; }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { पटिनिवत्त m_cost; }
    VBoolOrUnknown isConst() ध्रुव { पटिनिवत्त m_isConst; }
    शून्य isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    शून्य isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ध्रुव { पटिनिवत्त m_isStatic; }
    शून्य isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ध्रुव VL_MT_SAFE { पटिनिवत्त m_isTrace; }
    शून्य isTrace(bool flag) { m_isTrace = flag; }
    शून्य cname(ध्रुव string& name) { m_cname = name; }
    string cname() ध्रुव { पटिनिवत्त m_cname; }
    AstScope* scopep() ध्रुव { पटिनिवत्त m_scopep; }
    शून्य scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ध्रुव { पटिनिवत्त ((m_rtnType == "") ? "void" : m_rtnType); }
    शून्य rtnType(ध्रुव string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ध्रुव { पटिनिवत्त m_dontCombine || isTrace() || entryPoint(); }
    शून्य dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() ध्रुव { पटिनिवत्त m_declPrivate; }
    शून्य declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() ध्रुव VL_MT_SAFE { पटिनिवत्त m_keepIfEmpty; }
    शून्य keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() ध्रुव VL_MT_SAFE { पटिनिवत्त m_slow; }
    शून्य slow(bool flag) { m_slow = flag; }
    bool funcPublic() ध्रुव { पटिनिवत्त m_funcPublic; }
    शून्य funcPublic(bool flag) { m_funcPublic = flag; }
    शून्य argTypes(ध्रुव string& str) { m_argTypes = str; }
    string argTypes() ध्रुव { पटिनिवत्त m_argTypes; }
    शून्य ifdef(ध्रुव string& str) { m_ifdef = str; }
    string ifdef() ध्रुव { पटिनिवत्त m_ifdef; }
    bool isConstructor() ध्रुव { पटिनिवत्त m_isConstructor; }
    शून्य isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ध्रुव { पटिनिवत्त m_isDestructor; }
    शून्य isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() ध्रुव { पटिनिवत्त m_isMethod; }
    शून्य isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() ध्रुव { पटिनिवत्त m_isLoose; }
    शून्य isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() ध्रुव { पटिनिवत्त isMethod() && !isLoose(); }
    bool isVirtual() ध्रुव { पटिनिवत्त m_isVirtual; }
    शून्य isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ध्रुव { पटिनिवत्त m_needProcess; }
    शून्य setNeedProcess() { m_needProcess = true; }
    bool entryPoint() ध्रुव { पटिनिवत्त m_entryPoint; }
    शून्य entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() ध्रुव { पटिनिवत्त m_dpiPure; }
    शून्य dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() ध्रुव { पटिनिवत्त m_dpiContext; }
    शून्य dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ध्रुव VL_MT_SAFE { पटिनिवत्त m_dpiExportDispatcher; }
    शून्य dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ध्रुव { पटिनिवत्त m_dpiExportImpl; }
    शून्य dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ध्रुव VL_MT_SAFE { पटिनिवत्त m_dpiImportPrototype; }
    शून्य dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ध्रुव { पटिनिवत्त m_dpiImportWrapper; }
    शून्य dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ध्रुव { पटिनिवत्त !m_dpiCDecl.empty(); }
    ध्रुव string& dpiCDecl() ध्रुव { पटिनिवत्त m_dpiCDecl; }
    शून्य dpiCDecl(ध्रुव string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ध्रुव { पटिनिवत्त m_rtnType == "VlCoroutine"; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    bool recursive() ध्रुव { पटिनिवत्त m_recursive; }
    शून्य unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ध्रुव override { पटिनिवत्त m_unlikely; }  // Note virtual override
    शून्य noLife(bool flag) { m_noLife = flag; }
    bool noLife() ध्रुव { पटिनिवत्त m_noLife; }
    bool isCovergroupSample() ध्रुव { पटिनिवत्त m_isCovergroupSample; }
    शून्य isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ध्रुव { पटिनिवत्त m_voidSelf; }
    शून्य voidSelf(bool flag) { m_voidSelf = flag; }
    शून्य cost(पूर्णाङ्क cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() ध्रुव {
        पटिनिवत्त !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : सार्वजनिक Ast_ग्रन्थिः {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ध्रुव string m_name;
    ध्रुव VUseType m_useType;  // What sort of use this is

सार्वजनिक:
    AstCUse(File_पङ्क्तिः* fl, VUseType useType, ध्रुव string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    VUseType useType() ध्रुव { पटिनिवत्त m_useType; }
};
class AstCell final : सार्वजनिक Ast_ग्रन्थिः {
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
    File_पङ्क्तिः* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    पूर्णाङ्क m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
सार्वजनिक:
    AstCell(File_पङ्क्तिः* fl, File_पङ्क्तिः* mfl, ध्रुव string& instName, ध्रुव string& modName,
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
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    // ACCESSORS
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Cell name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string origName() ध्रुव override { पटिनिवत्त m_origName; }  // * = Original name
    शून्य origName(ध्रुव string& name) { m_origName = name; }
    string modName() ध्रुव { पटिनिवत्त m_modName; }  // * = Instance name
    शून्य modName(ध्रुव string& name) { m_modName = name; }
    File_पङ्क्तिः* modNameFileline() ध्रुव { पटिनिवत्त m_modNameFileline; }
    Ast_ग्रन्थिः_Module* modp() ध्रुव { पटिनिवत्त m_modp; }  // [AfterLink] = Pointer to module instantiated
    शून्य modp(Ast_ग्रन्थिः_Module* nodep) { m_modp = nodep; }
    पूर्णाङ्क arrayIdx() ध्रुव { पटिनिवत्त m_arrayIdx; }
    शून्य arrayIdx(पूर्णाङ्क idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ध्रुव { पटिनिवत्त m_hasIfaceVar; }
    शून्य hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    शून्य trace(bool flag) { m_trace = flag; }
    bool isTrace() ध्रुव { पटिनिवत्त m_trace; }
    शून्य recursive(bool flag) { m_recursive = flag; }
    bool recursive() ध्रुव { पटिनिवत्त m_recursive; }
};
class AstCellInline final : सार्वजनिक Ast_ग्रन्थिः {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ध्रुव std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ध्रुव std::string m_origModName;  // __BEGIN__ or __FORK__
सार्वजनिक:
    AstCellInline(File_पङ्क्तिः* fl, ध्रुव std::string& name, ध्रुव std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    // ACCESSORS
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    std::string origModName() ध्रुव { पटिनिवत्त m_origModName; }
};
class AstCellInlineScope final : सार्वजनिक Ast_ग्रन्थिः {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ध्रुव std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
सार्वजनिक:
    AstCellInlineScope(File_पङ्क्तिः* fl, ध्रुव std::string& name, ध्रुव std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    // ACCESSORS
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    शून्य name(ध्रुव std::string& name) override { m_name = name; }
    std::string origModName() ध्रुव { पटिनिवत्त m_origModName; }
};
class AstCgOptionAssign final : सार्वजनिक Ast_ग्रन्थिः {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ध्रुव VCoverOptionType m_optType;  // Option type
    ध्रुव string m_name;  // Original option name (for diagnostics on unknown options)
    ध्रुव bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
सार्वजनिक:
    AstCgOptionAssign(File_पङ्क्तिः* fl, bool typeOption, VCoverOptionType optType,
                      ध्रुव string& rawName, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    VCoverOptionType optType() ध्रुव { पटिनिवत्त m_optType; }
    bool typeOption() ध्रुव { पटिनिवत्त m_typeOption; }
};
class AstClassExtends final : सार्वजनिक Ast_ग्रन्थिः {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ध्रुव bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

सार्वजनिक:
    AstClassExtends(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    string verilogKwd() ध्रुव override { पटिनिवत्त isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ध्रुव;
    AstClass* classp() ध्रुव;  // Like above, but throws error if nulll
    bool isImplements() ध्रुव { पटिनिवत्त m_isImplements; }
    शून्य parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ध्रुव { पटिनिवत्त m_parameterized; }
};
class AstClocking final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ध्रुव bool m_isGlobal;  // True if global clocking

सार्वजनिक:
    AstClocking(File_पङ्क्तिः* fl, ध्रुव std::string& name, Ast_Sen_वृक्षः* sentreep, Ast_ग्रन्थिः* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    bool isDefault() ध्रुव { पटिनिवत्त m_isDefault; }
    bool isGlobal() ध्रुव { पटिनिवत्त m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    शून्य makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

सार्वजनिक:
    AstClockingItem(File_पङ्क्तिः* fl, VDirection direction, Ast_ग्रन्थिः_व्यञ्जकम्* skewp, Ast_ग्रन्थिः* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        सचे (AstAssign* ध्रुव clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } अञ्ञथा {
            exprp(VN_AS(clockingDeclp, ग्रन्थिः_व्यञ्जकम्));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VDirection direction() ध्रुव { पटिनिवत्त m_direction; }
    AstClockingItem* outputp() ध्रुव { पटिनिवत्त m_outputp; }
    शून्य outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
};
class AstConfig final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

सार्वजनिक:
    AstConfig(File_पङ्क्तिः* fl, ध्रुव std::string& libname, ध्रुव std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_libname + "." + m_configname; }
    std::string libname() ध्रुव VL_MT_STABLE { पटिनिवत्त m_libname; }
    std::string configname() ध्रुव VL_MT_STABLE { पटिनिवत्त m_configname; }
};
class AstConfigCell final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

सार्वजनिक:
    AstConfigCell(File_पङ्क्तिः* fl, ध्रुव std::string& libname, ध्रुव std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_libname + "." + m_cellname; }
    std::string libname() ध्रुव VL_MT_STABLE { पटिनिवत्त m_libname; }
    std::string cellname() ध्रुव VL_MT_STABLE { पटिनिवत्त m_cellname; }
};
class AstConfigRule final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ध्रुव bool m_isCell;  // Declared as "cell" versus "instance"

सार्वजनिक:
    AstConfigRule(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* cellp, Ast_ग्रन्थिः* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() ध्रुव VL_MT_STABLE { पटिनिवत्त m_isCell; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstConfigUse final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ध्रुव bool m_isConfig;  // ":config"; Config, not module/primitive name

सार्वजनिक:
    AstConfigUse(File_पङ्क्तिः* fl, ध्रुव std::string& libname, ध्रुव std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_libname + "." + m_cellname; }
    std::string libname() ध्रुव VL_MT_STABLE { पटिनिवत्त m_libname; }
    std::string cellname() ध्रुव VL_MT_STABLE { पटिनिवत्त m_cellname; }
    bool isConfig() ध्रुव VL_MT_STABLE { पटिनिवत्त m_isConfig; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstConstraint final : सार्वजनिक Ast_ग्रन्थिः {
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
    AstConstraint(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Scope name
    शून्य name(ध्रुव string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ध्रुव override { पटिनिवत्त false; }
    bool isPredictOptimizable() ध्रुव override { पटिनिवत्त false; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { पटिनिवत्त true; }
    शून्य baseOverride(ध्रुव VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ध्रुव { पटिनिवत्त m_baseOverride; }
    bool isExternDef() ध्रुव { पटिनिवत्त m_isExternDef; }
    शून्य isExternDef(bool flag) { m_isExternDef = flag; }
    शून्य isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ध्रुव { पटिनिवत्त m_isExternExplicit; }
    शून्य isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ध्रुव { पटिनिवत्त m_isExternProto; }
    शून्य isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() ध्रुव { पटिनिवत्त m_isKwdPure; }
    शून्य isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ध्रुव { पटिनिवत्त m_isStatic; }
};
class AstCoverBin final : सार्वजनिक Ast_ग्रन्थिः {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    ध्रुव string m_name;  // Base name of the bin
    ध्रुव VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

सार्वजनिक:
    AstCoverBin(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_ग्रन्थिः_व्यञ्जकम्* iffp = nullptr)
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
    AstCoverBin(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_व्यञ्जकम्* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_पङ्क्तिः* fl, ध्रुव string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_पङ्क्तिः* fl, ध्रुव string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    VCoverBinsType binsType() ध्रुव { पटिनिवत्त m_binsType; }
    bool isWildcard() ध्रुव { पटिनिवत्त m_isWildcard; }
    bool isArray() ध्रुव { पटिनिवत्त m_isArray; }
    शून्य isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : सार्वजनिक Ast_ग्रन्थिः {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ध्रुव bool m_isNegated;  // Complement the selection within the cross product

सार्वजनिक:
    AstCoverBinsof(File_पङ्क्तिः* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_ग्रन्थिः* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    bool isNegated() ध्रुव { पटिनिवत्त m_isNegated; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {  // LCOV_EXCL_START
        ध्रुव AstCoverBinsof* ध्रुव asamep = VN_DBG_AS(samep, CoverBinsof);
        पटिनिवत्त m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : सार्वजनिक Ast_ग्रन्थिः {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ध्रुव string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ध्रुव VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

सार्वजनिक:
    AstCoverCrossBin(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* selectp, Ast_ग्रन्थिः_व्यञ्जकम्* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    string verilogKwd() ध्रुव override;
    VCoverBinsType binsType() ध्रुव { पटिनिवत्त m_binsType; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {  // LCOV_EXCL_START
        ध्रुव AstCoverCrossBin* ध्रुव asamep = VN_DBG_AS(samep, CoverCrossBin);
        पटिनिवत्त m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : सार्वजनिक Ast_ग्रन्थिः {
    // The complete bin-tuple space of the enclosing cross
    ध्रुव string m_name;  // Referenced cross name

सार्वजनिक:
    AstCoverCrossRef(File_पङ्क्तिः* fl, ध्रुव string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {  // LCOV_EXCL_START
        पटिनिवत्त m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : सार्वजनिक Ast_ग्रन्थिः {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ध्रुव bool m_isOr;  // Union (||), rather than intersection (&&)

सार्वजनिक:
    AstCoverCrossSelect(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* lhsp, Ast_ग्रन्थिः* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool isOr() ध्रुव { पटिनिवत्त m_isOr; }
    string verilogKwd() ध्रुव override { पटिनिवत्त isOr() ? "||" : "&&"; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {  // LCOV_EXCL_START
        पटिनिवत्त m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : सार्वजनिक Ast_ग्रन्थिः {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
सार्वजनिक:
    AstCoverCrossWith(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* selectp, Ast_ग्रन्थिः_व्यञ्जकम्* filterp, Ast_ग्रन्थिः_व्यञ्जकम्* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : सार्वजनिक Ast_ग्रन्थिः {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ध्रुव VCoverOptionType m_optType;  // Option being assigned
    ध्रुव bool m_typeOption;  // type_option (vs option)

सार्वजनिक:
    AstCoverOption(File_पङ्क्तिः* fl, bool typeOption, VCoverOptionType optType, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VCoverOptionType optType() ध्रुव { पटिनिवत्त m_optType; }
    bool typeOption() ध्रुव { पटिनिवत्त m_typeOption; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {  // LCOV_EXCL_START
        ध्रुव AstCoverOption* ध्रुव asamep = VN_DBG_AS(samep, CoverOption);
        पटिनिवत्त m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : सार्वजनिक Ast_ग्रन्थिः {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
सार्वजनिक:
    AstCoverToggle(File_पङ्क्तिः* fl, AstCoverInc* incp, Ast_ग्रन्थिः_व्यञ्जकम्* origp, Ast_ग्रन्थिः_व्यञ्जकम्* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    पूर्णाङ्क instr_गणनम्() ध्रुव override { पटिनिवत्त 3 + INSTR_गणनम्_BRANCH + INSTR_गणनम्_LD; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { पटिनिवत्त true; }
    bool isGateOptimizable() ध्रुव override { पटिनिवत्त false; }
    bool isPredictOptimizable() ध्रुव override { पटिनिवत्त true; }
    bool isOutputter() override {
        पटिनिवत्त false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : सार्वजनिक Ast_ग्रन्थिः {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ध्रुव VTransRepType m_repType;

सार्वजनिक:
    AstCoverTransItem(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstCoverTransSet final : सार्वजनिक Ast_ग्रन्थिः {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
सार्वजनिक:
    AstCoverTransSet(File_पङ्क्तिः* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstCoverWith final : सार्वजनिक Ast_ग्रन्थिः {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
सार्वजनिक:
    AstCoverWith(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* subp, Ast_ग्रन्थिः_व्यञ्जकम्* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : सार्वजनिक Ast_ग्रन्थिः {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

सार्वजनिक:
    AstCovergroup(File_पङ्क्तिः* fl, ध्रुव string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_ग्रन्थिः* membersp, Ast_Sen_वृक्षः* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    bool maybePointedTo() ध्रुव override { पटिनिवत्त true; }
};
class AstCoverpointRef final : सार्वजनिक Ast_ग्रन्थिः {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ध्रुव string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

सार्वजनिक:
    AstCoverpointRef(File_पङ्क्तिः* fl, ध्रुव string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
};
class AstDefParam final : सार्वजनिक Ast_ग्रन्थिः {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
सार्वजनिक:
    AstDefParam(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* pathp, ध्रुव string& name, Ast_ग्रन्थिः_व्यञ्जकम्* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { पटिनिवत्त true; }
};
class AstDefaultClocking final : सार्वजनिक Ast_ग्रन्थिः {
    std::string m_name;  // Clocking block name

सार्वजनिक:
    AstDefaultClocking(File_पङ्क्तिः* fl, ध्रुव std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
};
class AstDefaultDisable final : सार्वजनिक Ast_ग्रन्थिः {
    // @astgen op1 := condp : AstNodeExpr

सार्वजनिक:
    AstDefaultDisable(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : सार्वजनिक Ast_ग्रन्थिः {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
सार्वजनिक:
    AstDpiExport(File_पङ्क्तिः* fl, ध्रुव string& vname, ध्रुव string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string cname() ध्रुव { पटिनिवत्त m_cname; }
    शून्य cname(ध्रुव string& cname) { m_cname = cname; }
};
class AstElabDisplay final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

सार्वजनिक:
    अन्तःस्थ AstElabDisplay(File_पङ्क्तिः* fl, VDisplayType dispType, Ast_ग्रन्थिः_व्यञ्जकम्* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!fmtp());
        पटिनिवत्त nullptr;
    }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string verilogKwd() ध्रुव override { पटिनिवत्त "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ध्रुव override { पटिनिवत्त false; }
    bool isPredictOptimizable() ध्रुव override { पटिनिवत्त false; }
    bool isPure() override { पटिनिवत्त false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { पटिनिवत्त true; }  // SPECIAL: $display makes output
    bool isUnlikely() ध्रुव override { पटिनिवत्त true; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        पटिनिवत्त displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    पूर्णाङ्क instr_गणनम्() ध्रुव override { पटिनिवत्त INSTR_गणनम्_PLI; }
    VDisplayType displayType() ध्रुव { पटिनिवत्त m_displayType; }
    शून्य displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : सार्वजनिक Ast_ग्रन्थिः {
    // Represents something missing, e.g. a missing argument in FOREACH
सार्वजनिक:
    explicit AstEmpty(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { पटिनिवत्त true; }
};
class AstGenCaseItem final : सार्वजनिक Ast_ग्रन्थिः {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
सार्वजनिक:
    AstGenCaseItem(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condsp, Ast_ग्रन्थिः* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() ध्रुव { पटिनिवत्त condsp() == nullptr; }
};
class AstImplicit final : सार्वजनिक Ast_ग्रन्थिः {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
सार्वजनिक:
    AstImplicit(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : सार्वजनिक Ast_ग्रन्थिः {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
सार्वजनिक:
    AstIntfRef(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& parentName, ध्रुव string& baseName,
               ध्रुव string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    string parentName() ध्रुव { पटिनिवत्त m_parentName; }
    string baseName() ध्रुव { पटिनिवत्त m_baseName; }
    string modportName() ध्रुव { पटिनिवत्त m_modportName; }
};
class AstLibrary final : सार्वजनिक Ast_ग्रन्थिः {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

सार्वजनिक:
    AstLibrary(File_पङ्क्तिः* fl, ध्रुव std::string& name, Ast_ग्रन्थिः_व्यञ्जकम्* filesp, Ast_ग्रन्थिः_व्यञ्जकम्* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
};
class AstModport final : सार्वजनिक Ast_ग्रन्थिः {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
सार्वजनिक:
    AstModport(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() ध्रुव override { पटिनिवत्त "modport"; }
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : सार्वजनिक Ast_ग्रन्थिः {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
सार्वजनिक:
    AstModportClockingRef(File_पङ्क्तिः* fl, ध्रुव string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    शून्य dump(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    AstClocking* clockingp() ध्रुव VL_MT_STABLE {
        पटिनिवत्त m_clockingp;
    }  // [After Link] Pointer to clocking block
    शून्य clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : सार्वजनिक Ast_ग्रन्थिः {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
सार्वजनिक:
    AstModportFTaskRef(File_पङ्क्तिः* fl, ध्रुव string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    bool isImport() ध्रुव { पटिनिवत्त !m_export; }
    bool isExport() ध्रुव { पटिनिवत्त m_export; }
    Ast_ग्रन्थिः_FTask* ftaskp() ध्रुव { पटिनिवत्त m_ftaskp; }  // [After Link] Pointer to variable
    शून्य ftaskp(Ast_ग्रन्थिः_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : सार्वजनिक Ast_ग्रन्थिः {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ध्रुव VDirection m_direction;  // Direction of the variable (in/out)
सार्वजनिक:
    AstModportVarRef(File_पङ्क्तिः* fl, ध्रुव string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    VDirection direction() ध्रुव { पटिनिवत्त m_direction; }
    AstVar* varp() ध्रुव VL_MT_STABLE { पटिनिवत्त m_varp; }  // [After Link] Pointer to variable
    शून्य varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : सार्वजनिक Ast_ग्रन्थिः {
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
    std::string m_resolved_शिखरम्_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ध्रुव AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ध्रुव Ast_ग्रन्थिः*, ध्रुव Ast_ग्रन्थिः_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

सार्वजनिक:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ध्रुव वर्ण* broken() ध्रुव override;
    शून्य प्रक्षिप_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.निवेशय(varp); }
    ध्रुव std::set<AstVar*>& deferredParamVarps() ध्रुव { पटिनिवत्त m_deferredParamVarps; }
    शून्य clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    शून्य deleteContents();
    शून्य cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त "$root"; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    Ast_ग्रन्थिः_Module* शिखरम्_Modulep() ध्रुव VL_MT_STABLE {  // Top module in hierarchy
        पटिनिवत्त modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { पटिनिवत्त m_typeTablep; }
    string astConstOrigParamName(ध्रुव AstConst* nodep) ध्रुव;
    शून्य astConstOrigParamName(ध्रुव AstConst* nodep, ध्रुव string& name);
    शून्य astConstOrigParamNameErase(ध्रुव AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ध्रुव Ast_ग्रन्थिः_Module* containingModule(ध्रुव Ast_ग्रन्थिः* nodep);
    // Forget remembered modules, as the tree has moved.
    शून्य clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ध्रुव { पटिनिवत्त m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ध्रुव { पटिनिवत्त m_dollarUnitPkgp; }
    शून्य dollarUnitPkgp(AstPackage* ध्रुव packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ध्रुव { पटिनिवत्त m_dpiExportTriggerp; }
    शून्य dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ध्रुव { पटिनिवत्त m_delaySchedulerp; }
    शून्य delaySchedulerp(AstVar* ध्रुव varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ध्रुव { पटिनिवत्त m_nbaEventp; }
    शून्य nbaEventp(AstVarScope* ध्रुव varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ध्रुव { पटिनिवत्त m_nbaEventTriggerp; }
    शून्य nbaEventTriggerp(AstVarScope* ध्रुव varScopep) { m_nbaEventTriggerp = varScopep; }
    शून्य stdPackagep(AstPackage* ध्रुव packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ध्रुव { पटिनिवत्त m_stdPackagep; }
    शून्य stdPackageProcessp(AstClass* ध्रुव classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ध्रुव { पटिनिवत्त m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_पङ्क्तिः*) ध्रुव;
    Ast_शिखरम्_Scope* शिखरम्_Scopep() ध्रुव { पटिनिवत्त m_शिखरम्_Scopep; }
    शून्य create_शिखरम्_Scope(AstScope* scopep);
    VTimescale timeunit() ध्रुव { पटिनिवत्त m_timeunit; }
    शून्य timeunit(ध्रुव VTimescale& मानम्) { m_timeunit = मानम्; }
    VTimescale timeprecision() ध्रुव { पटिनिवत्त m_timeprecision; }
    शून्य timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    शून्य timeprecisionMerge(File_पङ्क्तिः*, ध्रुव VTimescale& मानम्);
    शून्य timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ध्रुव { पटिनिवत्त m_timescaleSpecified; }
    ध्रुव std::string& resolved_शिखरम्_Module_Name() ध्रुव { पटिनिवत्त m_resolved_शिखरम्_Module_Name; }
    शून्य resolved_शिखरम्_Module_Name(ध्रुव std::string& मानम्) { m_resolved_शिखरम्_Module_Name = मानम्; }
    uint32_t nTraceCodes() ध्रुव { पटिनिवत्त m_nTraceCodes; }
    शून्य nTraceCodes(uint32_t मानम्) { m_nTraceCodes = मानम्; }
    AstVarScope* stlFirstIterationp();
    शून्य clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    ध्रुव std::string trace_Lib_शिखरम्_Name() ध्रुव {
        ध्रुव std::string& name = resolved_शिखरम्_Module_Name();
        पटिनिवत्त prettyName(name.empty() ? v3Global.rootp()->शिखरम्_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    शून्य addEvalStats(ध्रुव std::string& phase);
};
class AstPackageExport final : सार्वजनिक Ast_ग्रन्थिः {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

सार्वजनिक:
    AstPackageExport(File_पङ्क्तिः* fl, ध्रुव string& pkgName, ध्रुव string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    string pkgName() ध्रुव VL_MT_STABLE { पटिनिवत्त m_pkgName; }
    string prettyPkgNameQ() ध्रुव { पटिनिवत्त "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ध्रुव { पटिनिवत्त m_packagep; }
    शून्य packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : सार्वजनिक Ast_ग्रन्थिः {
    // A package export *::* declaration
सार्वजनिक:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : सार्वजनिक Ast_ग्रन्थिः {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

सार्वजनिक:
    AstPackageImport(File_पङ्क्तिः* fl, AstPackage* packagep, ध्रुव string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_पङ्क्तिः* fl, ध्रुव string& pkgName, ध्रुव string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    string pkgName() ध्रुव VL_MT_STABLE { पटिनिवत्त m_pkgName; }
    string prettyPkgNameQ() ध्रुव { पटिनिवत्त "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ध्रुव { पटिनिवत्त m_packagep; }
    शून्य packagep(AstPackage* nodep) { m_packagep = nodep; }

गुप्त:
    शून्य pkgNameFrom();
};
class AstPin final : सार्वजनिक Ast_ग्रन्थिः {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    पूर्णाङ्क m_pin_अङ्कः;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
सार्वजनिक:
    AstPin(File_पङ्क्तिः* fl, पूर्णाङ्क pin_अङ्कः, ध्रुव string& name, Ast_ग्रन्थिः* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_अङ्कः{pin_अङ्कः}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    शून्य cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Pin name, ""=go by number
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string prettyOperatorName() ध्रुव override;
    bool dotStar() ध्रुव { पटिनिवत्त name() == ".*"; }  // Fake name for .* connections until linked
    पूर्णाङ्क pin_अङ्कः() ध्रुव { पटिनिवत्त m_pin_अङ्कः; }
    AstVar* modVarp() ध्रुव { पटिनिवत्त m_modVarp; }  // [After Link] Pointer to variable
    शून्य modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ध्रुव { पटिनिवत्त m_modPTypep; }
    शून्य modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ध्रुव { पटिनिवत्त m_param; }
    शून्य param(bool flag) { m_param = flag; }
    ध्रुव string& paramPath() ध्रुव { पटिनिवत्त m_paramPath; }
    शून्य paramPath(ध्रुव string& path) { m_paramPath = path; }
    bool svDotName() ध्रुव { पटिनिवत्त m_svDotName; }
    शून्य svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ध्रुव { पटिनिवत्त m_svImplicit; }
    शून्य svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : सार्वजनिक Ast_ग्रन्थिः {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ध्रुव पूर्णाङ्क m_pin_अङ्कः;  // Pin number
    ध्रुव string m_name;  // Name of pin
सार्वजनिक:
    AstPort(File_पङ्क्तिः* fl, पूर्णाङ्क pinnum, ध्रुव string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_अङ्कः{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Port name
    पूर्णाङ्क pin_अङ्कः() ध्रुव { पटिनिवत्त m_pin_अङ्कः; }  // * = Pin number, for order based instantiation
};
class AstPragma final : सार्वजनिक Ast_ग्रन्थिः {
    ध्रुव VPragmaType m_pragType;  // Type of pragma
    ध्रुव VTimescale m_timescale;  // For TIMEUNIT_SET
सार्वजनिक:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_पङ्क्तिः* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_पङ्क्तिः* fl, ध्रुव VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VPragmaType pragType() ध्रुव { पटिनिवत्त m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ध्रुव override { पटिनिवत्त false; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        पटिनिवत्त pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ध्रुव { पटिनिवत्त m_timescale; }
};
class AstPropSpec final : सार्वजनिक Ast_ग्रन्थिः {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

सार्वजनिक:
    AstPropSpec(File_पङ्क्तिः* fl, AstSenItem* sensesp, Ast_ग्रन्थिः_व्यञ्जकम्* disablep, Ast_ग्रन्थिः* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool hasDType() ध्रुव override VL_MT_SAFE {
        पटिनिवत्त true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ध्रुव { पटिनिवत्त m_propStrength; }
};
class AstPull final : सार्वजनिक Ast_ग्रन्थिः {
    // @astgen op1 := lhsp : AstNodeExpr

    ध्रुव bool m_direction;

सार्वजनिक:
    AstPull(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        पटिनिवत्त direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ध्रुव { पटिनिवत्त m_direction; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstScope final : सार्वजनिक Ast_ग्रन्थिः {
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
    AstScope(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Module* modp, ध्रुव string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!m_modp);
        पटिनिवत्त nullptr;
    }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Scope name
    शून्य name(ध्रुव string& name) override { m_name = name; }
    string pretty_Name_सन्देशः() ध्रुव override;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override;
    string nameDotless() ध्रुव;
    Ast_ग्रन्थिः_Module* modp() ध्रुव { पटिनिवत्त m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() ध्रुव VL_MT_SAFE { पटिनिवत्त m_aboveScopep; }
    शून्य aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ध्रुव { पटिनिवत्त m_aboveCellp; }
    शून्य aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_शिखरम्() ध्रुव VL_MT_SAFE { पटिनिवत्त aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : सार्वजनिक Ast_ग्रन्थिः {
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
    AstSenItem(File_पङ्क्तिः* fl, VEdgeType edgeType, Ast_ग्रन्थिः_व्यञ्जकम्* senp, Ast_ग्रन्थिः_व्यञ्जकम्* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_पङ्क्तिः* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_पङ्क्तिः* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_पङ्क्तिः* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_पङ्क्तिः* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_पङ्क्तिः* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_पङ्क्तिः* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        पटिनिवत्त edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() ध्रुव { पटिनिवत्त m_edgeType; }
    शून्य edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_गणनम्_Inc();
    }
    Ast_ग्रन्थिः_Var_Ref* varrefp() ध्रुव { पटिनिवत्त VN_CAST(sensp(), ग्रन्थिः_Var_Ref); }
    //
    bool isClocked() ध्रुव { पटिनिवत्त edgeType().clockedStmt(); }
    bool isComboOrStar() ध्रुव {
        पटिनिवत्त edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() ध्रुव { पटिनिवत्त edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() ध्रुव { पटिनिवत्त edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() ध्रुव { पटिनिवत्त edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() ध्रुव { पटिनिवत्त edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() ध्रुव { पटिनिवत्त edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() ध्रुव { पटिनिवत्त edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_वृक्षः final : सार्वजनिक Ast_ग्रन्थिः {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
सार्वजनिक:
    Ast_Sen_वृक्षः(File_पङ्क्तिः* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_वृक्षः(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_वृक्षः;
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    // METHODS
    bool hasClocked() ध्रुव;  // Includes a clocked statement
    bool hasEdge() ध्रुव;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ध्रुव;  // Includes a STATIC SenItem
    bool hasInitial() ध्रुव;  // Includes a INITIAL SenItem
    bool hasFinal() ध्रुव;  // Includes a FINAL SenItem
    bool hasCombo() ध्रुव;  // Includes a COMBO SenItem
    bool hasHybrid() ध्रुव;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : सार्वजनिक Ast_ग्रन्थिः {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

सार्वजनिक:
    AstStrengthSpec(File_पङ्क्तिः* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { पटिनिवत्त m_s0; }
    VStrength strength1() { पटिनिवत्त m_s1; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstSystemCSection final : सार्वजनिक Ast_ग्रन्थिः {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ध्रुव VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

सार्वजनिक:
    AstSystemCSection(File_पङ्क्तिः* fl, VSystemCSectionType sectionType, ध्रुव std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ध्रुव { पटिनिवत्त m_sectionType; }
    ध्रुव std::string& text() ध्रुव { पटिनिवत्त m_text; }
    शून्य text(ध्रुव std::string& मानम्) { m_text = मानम्; }
    शून्य dump(std::ostream&) ध्रुव override;
    शून्य dumpJson(std::ostream&) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः*) ध्रुव override { पटिनिवत्त false; }
    bool isPure() override { पटिनिवत्त false; }
    bool isOutputter() override { पटिनिवत्त true; }
};
class AstText final : सार्वजनिक Ast_ग्रन्थिः {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ध्रुव std::string m_text;  // The text to emit
सार्वजनिक:
    AstText(File_पङ्क्तिः* fl, ध्रुव std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        पटिनिवत्त text() == VN_DBG_AS(samep, Text)->text();
    }
    ध्रुव std::string& text() ध्रुव VL_MT_SAFE { पटिनिवत्त m_text; }
};
class AstTextBlock final : सार्वजनिक Ast_ग्रन्थिः {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ध्रुव std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ध्रुव std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ध्रुव std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
सार्वजनिक:
    explicit AstTextBlock(File_पङ्क्तिः* fl,  //
                          ध्रुव std::string& prefix = "",  //
                          ध्रुव std::string& separator = "",  //
                          ध्रुव std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    ध्रुव std::string& prefix() ध्रुव { पटिनिवत्त m_prefix; }
    ध्रुव std::string& separator() ध्रुव { पटिनिवत्त m_separator; }
    ध्रुव std::string& suffix() ध्रुव { पटिनिवत्त m_suffix; }
    // Add some text, or a node to this block
    शून्य add(ध्रुव string& text) { addNodesp(new AstText{fileline(), text}); }
    शून्य add(Ast_ग्रन्थिः* nodep) { addNodesp(nodep); }
};
class Ast_शिखरम्_Scope final : सार्वजनिक Ast_ग्रन्थिः {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    friend class AstNetlist;  // Only the AstNetlist can create one
    Ast_शिखरम्_Scope(File_पङ्क्तिः* fl, AstScope* ascopep)
        : ASTGEN_SUPER_शिखरम्_Scope(fl) {
        this->scopep(ascopep);
    }

सार्वजनिक:
    ASTGEN_MEMBERS_Ast_शिखरम्_Scope;
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* createTemp(ध्रुव string& name, unsigned width);
    AstVarScope* createTemp(ध्रुव string& name, Ast_ग्रन्थिः_DType* dtypep);
};
class AstTypeTable final : सार्वजनिक Ast_ग्रन्थिः {
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
    using DetailedMap = std::map<VBasic_Type_कुञ्चिका, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

सार्वजनिक:
    explicit AstTypeTable(File_पङ्क्तिः* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    शून्य cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_पङ्क्तिः* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_पङ्क्तिः* fl, VBasicDTypeKwd kwd, पूर्णाङ्क width, पूर्णाङ्क widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_पङ्क्तिः* fl, VBasicDTypeKwd kwd, ध्रुव VNumRange& range,
                                     पूर्णाङ्क widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& ग्रन्थिः);
    AstBasicDType* find_निवेशय_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_पङ्क्तिः* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_पङ्क्तिः* fl);
    AstQueueDType* findQueueIndexDType(File_पङ्क्तिः* fl);
    AstStreamDType* findStreamDType(File_पङ्क्तिः* fl);
    AstVoidDType* findVoidDType(File_पङ्क्तिः* fl);
    शून्य clearCache();
    शून्य repairCache();
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
};
class AstTypedef final : सार्वजनिक Ast_ग्रन्थिः {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_अङ्कः;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

सार्वजनिक:
    AstTypedef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* attrsp, VFlagChildDType,
               Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_अङ्कः{fl->token_अङ्कः()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_अङ्कः{fl->token_अङ्कः()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ध्रुव;
    Ast_ग्रन्थिः_DType* getChildDTypep() ध्रुव override { पटिनिवत्त childDTypep(); }
    आभास Ast_ग्रन्थिः_DType* subDTypep() ध्रुव VL_MT_STABLE {
        पटिनिवत्त dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_अङ्कः() ध्रुव override { पटिनिवत्त m_decl_Token_अङ्कः; }
    शून्य decl_Token_अङ्कः_Set_Min(uint32_t token_अङ्कः) override {
        m_decl_Token_अङ्कः = std::min(m_decl_Token_अङ्कः, token_अङ्कः);
    }
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    शून्य name(ध्रुव string& flag) override { m_name = flag; }
    bool attrPublic() ध्रुव { पटिनिवत्त m_attrPublic; }
    शून्य attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() ध्रुव { पटिनिवत्त m_isHideLocal; }
    शून्य isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ध्रुव { पटिनिवत्त m_isHideProtected; }
    शून्य isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ध्रुव { पटिनिवत्त m_isUnderClass; }
    शून्य isUnderClass(bool flag) { m_isUnderClass = flag; }
    शून्य tag(ध्रुव string& text) override { m_tag = text; }
    string tag() ध्रुव override { पटिनिवत्त m_tag; }
};
class AstTypedefFwd final : सार्वजनिक Ast_ग्रन्थिः {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ध्रुव VFwdType m_fwdType;  // Forward type for lint check

सार्वजनिक:
    AstTypedefFwd(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    VFwdType fwdType() ध्रुव { पटिनिवत्त m_fwdType; }
};
class AstUdpTable final : सार्वजनिक Ast_ग्रन्थिः {
    // @astgen op1 := linesp : List[AstUdpTableLine]
सार्वजनिक:
    AstUdpTable(File_पङ्क्तिः* fl, Ast_Udp_Table_पङ्क्तिः* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        सचे (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_पङ्क्तिः final : सार्वजनिक Ast_ग्रन्थिः {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
गुप्त:
    ध्रुव bool m_udpIsCombo;  // Combinational or sequential UDP

सार्वजनिक:
    class UdpCombo {};
    Ast_Udp_Table_पङ्क्तिः(UdpCombo, File_पङ्क्तिः* fl, Ast_Udp_Table_पङ्क्तिः_Val* iFieldsp,
                    Ast_Udp_Table_पङ्क्तिः_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_पङ्क्तिः(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_पङ्क्तिः(UdpSequential, File_पङ्क्तिः* fl, Ast_Udp_Table_पङ्क्तिः_Val* iFieldsp,
                    Ast_Udp_Table_पङ्क्तिः_Val* oFieldsp1, Ast_Udp_Table_पङ्क्तिः_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_पङ्क्तिः(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_पङ्क्तिः;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क udpIsCombo() ध्रुव { पटिनिवत्त m_udpIsCombo; }
};
class Ast_Udp_Table_पङ्क्तिः_Val final : सार्वजनिक Ast_ग्रन्थिः {
    string m_text;  // Value character

सार्वजनिक:
    Ast_Udp_Table_पङ्क्तिः_Val(File_पङ्क्तिः* fl, ध्रुव string& text)
        : ASTGEN_SUPER_Udp_Table_पङ्क्तिः_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_पङ्क्तिः_Val;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_text; }
    शून्य name(std::string ध्रुव& text) override VL_MT_STABLE { m_text = text; }
    string text() ध्रुव VL_MT_SAFE { पटिनिवत्त m_text; }
};
class AstVar final : सार्वजनिक Ast_ग्रन्थिः {
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
    पूर्णाङ्क m_pin_अङ्कः = 0;  // For JSON, if non-zero the connection pin number
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
    bool m_mtask_Cache_पङ्क्तिः_Align : 1;  // Start MTask affinity group on a cache line
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
        m_mtask_Cache_पङ्क्तिः_Align = false;
    }

सार्वजनिक:
    AstVar(File_पङ्क्तिः* fl, VVarType type, ध्रुव string& name, VFlagChildDType, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_पङ्क्तिः* fl, VVarType type, ध्रुव string& name, Ast_ग्रन्थिः_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_पङ्क्तिः* fl, VVarType type, ध्रुव string& name, VFlagLogicPacked, पूर्णाङ्क wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_पङ्क्तिः* fl, VVarType type, ध्रुव string& name, VFlagBitPacked, पूर्णाङ्क wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_पङ्क्तिः* fl, VVarType type, ध्रुव string& name, ध्रुव AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        सचे (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_वृक्षः(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    अन्तःस्थ bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override;
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }  // * = Var name
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    string origName() ध्रुव override { पटिनिवत्त m_origName; }  // * = Original name
    शून्य origName(ध्रुव string& name) { m_origName = name; }
    VVarType varType() ध्रुव VL_MT_SAFE { पटिनिवत्त m_varType; }  // * = Type of variable
    शून्य direction(ध्रुव VDirection& flag) {
        m_direction = flag;
        सचे (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ध्रुव VL_MT_SAFE { पटिनिवत्त m_direction; }
    bool isIO() ध्रुव VL_MT_SAFE { पटिनिवत्त m_direction != VDirection::NONE; }
    bool isVLIO() ध्रुव {
        ध्रुव AstBasicDType* ध्रुव bdtypep = basicp();
        पटिनिवत्त isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    शून्य declDirection(ध्रुव VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ध्रुव { पटिनिवत्त m_declDirection; }
    पूर्णाङ्क libInoutId() ध्रुव { पटिनिवत्त m_libInoutId; }
    शून्य libInoutId(पूर्णाङ्क id) { m_libInoutId = id; }
    bool libInoutEnable() ध्रुव { पटिनिवत्त m_libInoutEnable; }
    शून्य libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    शून्य varType(VVarType type) { m_varType = type; }
    शून्य varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    शून्य varType2In() {
        m_tristate = false;
        m_direction = VDirection::निवेशः;
    }
    string scType() ध्रुव;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) ध्रुव;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) ध्रुव;
    string dpiTmpVarType(ध्रुव string& varName) ध्रुव;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) ध्रुव;
    string vlEnumType() ध्रुव;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) ध्रुव;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(ध्रुव string& propName) ध्रुव;  // Return VerilatorVarProps declaration
    शून्य combineType(VVarType type);
    Ast_ग्रन्थिः_DType* getChildDTypep() ध्रुव override { पटिनिवत्त childDTypep(); }
    Ast_ग्रन्थिः_DType* dtypeSkipRefp() ध्रुव VL_MT_STABLE { पटिनिवत्त subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ध्रुव VL_MT_STABLE { पटिनिवत्त subDTypep()->basicp(); }
    आभास Ast_ग्रन्थिः_DType* subDTypep() ध्रुव VL_MT_STABLE {
        पटिनिवत्त dtypep() ? dtypep() : childDTypep();
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
    शून्य attrFsmStateExpand(ध्रुव VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() ध्रुव { पटिनिवत्त m_constPoolEntry; }
    शून्य setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() ध्रुव { पटिनिवत्त m_covergroupRefMember; }
    शून्य covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ध्रुव { पटिनिवत्त m_embeddedCovergroup; }
    शून्य embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    शून्य rand(ध्रुव VRandAttr flag) { m_rand = flag; }
    शून्य usedParam(bool flag) { m_usedParam = flag; }
    शून्य usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    शून्य sigPublic(bool flag) { m_sigPublic = flag; }
    शून्य sigModPublic(bool flag) { m_sigModPublic = flag; }
    शून्य sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        सचे (flag) sigPublic(true);
    }
    शून्य sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        सचे (flag) sigUserRdPublic(true);
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
        सचे (flag) m_funcLocalSticky = true;
    }
    शून्य funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ध्रुव { पटिनिवत्त m_gotNansiType; }
    शून्य gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ध्रुव { पटिनिवत्त m_hasStrengthAssignment; }
    शून्य hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ध्रुव { पटिनिवत्त m_hasUserInit; }
    शून्य hasUserInit(bool flag) { m_hasUserInit = flag; }
    शून्य icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ध्रुव { पटिनिवत्त m_icoMaybeWritten; }
    शून्य isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ध्रुव { पटिनिवत्त m_isClockingDriven; }
    bool isDpiOpenArray() ध्रुव VL_MT_SAFE { पटिनिवत्त m_isDpiOpenArray; }
    शून्य isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ध्रुव { पटिनिवत्त m_isHideLocal; }
    शून्य isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ध्रुव { पटिनिवत्त m_isHideProtected; }
    शून्य isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ध्रुव { पटिनिवत्त m_noCReset; }
    शून्य noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ध्रुव { पटिनिवत्त m_noReset; }
    शून्य noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ध्रुव { पटिनिवत्त m_noSubst; }
    शून्य noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() ध्रुव { पटिनिवत्त m_processQueue; }
    शून्य processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() ध्रुव { पटिनिवत्त m_sampled; }
    शून्य sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ध्रुव { पटिनिवत्त m_substConstOnly; }
    शून्य substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ध्रुव { पटिनिवत्त m_overriddenParam; }
    शून्य overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ध्रुव { पटिनिवत्त m_untypedParam; }
    शून्य untypedParam(bool flag) { m_untypedParam = flag; }
    शून्य trace(bool flag) { m_trace = flag; }
    शून्य isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ध्रुव { पटिनिवत्त m_isForceable; }
    शून्य setForceable() { m_isForceable = true; }
    शून्य setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() ध्रुव { पटिनिवत्त m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() ध्रुव { पटिनिवत्त m_isReadByDpi; }
    शून्य setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ध्रुव { पटिनिवत्त m_isWrittenByDpi; }
    शून्य setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ध्रुव { पटिनिवत्त m_isWrittenBySuspendable; }
    शून्य setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ध्रुव { पटिनिवत्त m_ignorePostRead; }
    शून्य setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() ध्रुव { पटिनिवत्त m_ignorePostWrite; }
    शून्य setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ध्रुव { पटिनिवत्त m_ignoreSchedWrite; }
    शून्य setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ध्रुव { पटिनिवत्त m_dfgTriLowered; }
    शून्य setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ध्रुव { पटिनिवत्त m_dfgAllowMultidriveTri; }
    शून्य setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    शून्य globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ध्रुव { पटिनिवत्त m_globalConstrained; }
    bool isStdRandomizeArg() ध्रुव { पटिनिवत्त m_isStdRandomizeArg; }
    शून्य setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    शून्य name(ध्रुव string& name) override { m_name = name; }
    शून्य tag(ध्रुव string& text) override { m_tag = text; }
    string tag() ध्रुव override { पटिनिवत्त m_tag; }
    bool isAnsi() ध्रुव { पटिनिवत्त m_ansi; }
    bool isContinuously() ध्रुव { पटिनिवत्त m_isContinuously; }
    bool isDeclTyped() ध्रुव { पटिनिवत्त m_declTyped; }
    bool isInout() ध्रुव { पटिनिवत्त m_direction.isInout(); }
    bool isInoutOrRef() ध्रुव { पटिनिवत्त m_direction.isInoutOrRef(); }
    bool is_निवेशः() ध्रुव { पटिनिवत्त m_direction.is_निवेशः(); }
    bool isNonOutput() ध्रुव { पटिनिवत्त m_direction.isNonOutput(); }
    bool isReadOnly() ध्रुव VL_MT_SAFE { पटिनिवत्त m_direction.isReadOnly(); }
    bool isConstRef() ध्रुव VL_MT_SAFE { पटिनिवत्त m_direction.isConstRef(); }
    bool isRef() ध्रुव VL_MT_SAFE { पटिनिवत्त m_direction.isRef(); }
    bool isWritable() ध्रुव VL_MT_SAFE { पटिनिवत्त m_direction.isWritable(); }
    bool isTristate() ध्रुव { पटिनिवत्त m_tristate; }
    bool isPrimaryIO() ध्रुव VL_MT_SAFE { पटिनिवत्त m_primaryIO; }
    bool isPrimaryInish() ध्रुव { पटिनिवत्त isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ध्रुव VL_MT_SAFE { पटिनिवत्त m_primaryClock; }
    शून्य setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() ध्रुव { पटिनिवत्त varType() == VVarType::IFACEREF; }
    शून्य setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() ध्रुव { पटिनिवत्त m_isIfaceParent; }
    bool isIfaceArraySplit() ध्रुव { पटिनिवत्त m_isIfaceArraySplit; }
    bool isInternal() ध्रुव { पटिनिवत्त m_isInternal; }
    bool isIfaceParam() ध्रुव { पटिनिवत्त m_isIfaceParam; }
    bool isSignal() ध्रुव { पटिनिवत्त varType().isSignal(); }
    bool isNet() ध्रुव { पटिनिवत्त varType().isNet(); }
    bool isWor() ध्रुव { पटिनिवत्त varType().isWor(); }
    bool isWiredNet() ध्रुव { पटिनिवत्त varType().isWiredNet(); }
    bool isTemp() ध्रुव { पटिनिवत्त varType().isTemp(); }
    bool isClassMember() ध्रुव { पटिनिवत्त varType() == VVarType::MEMBER; }
    bool isVirtIface() ध्रुव {
        सचे (AstIfaceRefDType* ध्रुव dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            पटिनिवत्त dtp->isVirtual();
        }
        पटिनिवत्त false;
    }
    bool isStatementTemp() ध्रुव { पटिनिवत्त varType() == VVarType::STMTTEMP; }
    bool isXTemp() ध्रुव { पटिनिवत्त varType() == VVarType::XTEMP; }
    bool isParam() ध्रुव { पटिनिवत्त varType().isParam(); }
    bool isGParam() ध्रुव { पटिनिवत्त varType() == VVarType::GPARAM; }
    bool isGenVar() ध्रुव { पटिनिवत्त varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ध्रुव {
        पटिनिवत्त isIO() || isSignal() || isClassMember() || isTemp() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ध्रुव {
        पटिनिवत्त !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() ध्रुव {
        ध्रुव AstBasicDType* ध्रुव bdtypep = basicp();
        पटिनिवत्त bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() ध्रुव { पटिनिवत्त m_usedParam; }
    bool isUsedLoopIdx() ध्रुव { पटिनिवत्त m_usedLoopIdx; }
    bool mtask_Cache_पङ्क्तिः_Align() ध्रुव { पटिनिवत्त m_mtask_Cache_पङ्क्तिः_Align; }
    शून्य mtask_Cache_पङ्क्तिः_Align(bool flag) { m_mtask_Cache_पङ्क्तिः_Align = flag; }
    bool isSc() ध्रुव VL_MT_SAFE { पटिनिवत्त m_sc; }
    bool isScQuad() ध्रुव;
    bool isScBv() ध्रुव VL_MT_STABLE;
    bool isScUint() ध्रुव;
    bool isScUintBool() ध्रुव;
    bool isScBigUint() ध्रुव VL_MT_STABLE;
    bool isScSensitive() ध्रुव { पटिनिवत्त m_scSensitive; }
    bool isSigPublic() ध्रुव;
    bool isSigModPublic() ध्रुव { पटिनिवत्त m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() ध्रुव { पटिनिवत्त m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() ध्रुव { पटिनिवत्त m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ध्रुव { पटिनिवत्त m_trace; }
    bool isRand() ध्रुव { पटिनिवत्त m_rand.isRand(); }
    bool isRandC() ध्रुव { पटिनिवत्त m_rand.isRandC(); }
    bool isConst() ध्रुव VL_MT_SAFE { पटिनिवत्त m_isConst; }
    bool isLatched() ध्रुव { पटिनिवत्त m_isLatched; }
    bool isFuncLocal() ध्रुव { पटिनिवत्त m_funcLocal; }
    bool isFuncLocalSticky() ध्रुव { पटिनिवत्त m_funcLocalSticky; }
    bool isFuncReturn() ध्रुव { पटिनिवत्त m_funcReturn; }
    bool isPullup() ध्रुव { पटिनिवत्त m_isPullup; }
    bool isPulldown() ध्रुव { पटिनिवत्त m_isPulldown; }
    bool attrScBv() ध्रुव { पटिनिवत्त m_attrScBv; }
    bool attrScBigUint() ध्रुव { पटिनिवत्त m_attrScBigUint; }
    bool attrFileDescr() ध्रुव { पटिनिवत्त m_attrFileDescr; }
    bool attrSFormat() ध्रुव { पटिनिवत्त m_attrSFormat; }
    bool attrSplitVar() ध्रुव { पटिनिवत्त m_attrSplitVar; }
    bool attrFsmState() ध्रुव { पटिनिवत्त m_attrFsmState; }
    bool attrFsmArcInclCond() ध्रुव { पटिनिवत्त m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ध्रुव { पटिनिवत्त m_attrFsmStateExpand; }
    AstIface* sensIfacep() ध्रुव { पटिनिवत्त m_sensIfacep; }
    VRandAttr rand() ध्रुव { पटिनिवत्त m_rand; }
    string verilogKwd() ध्रुव override;
    शून्य lifetime(ध्रुव VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ध्रुव { पटिनिवत्त m_lifetime; }
    शून्य pin_अङ्कः(पूर्णाङ्क id) { m_pin_अङ्कः = id; }
    पूर्णाङ्क pin_अङ्कः() ध्रुव { पटिनिवत्त m_pin_अङ्कः; }
    शून्य propagateAttrFrom(ध्रुव AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        सचे (fromp->attrFileDescr()) attrFileDescr(true);
        सचे (fromp->isContinuously()) isContinuously(true);
    }
    शून्य propagateSplitAttrFrom(ध्रुव AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ध्रुव वर्ण* cannotSplitKindReason() ध्रुव;
    शून्य propagateWrapAttrFrom(ध्रुव AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    शून्य combineType(ध्रुव AstVar* otherp);
    bool needsCReset() ध्रुव {
        पटिनिवत्त !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !isStatementTemp()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    स्थिर AstVar* scVarRecurse(Ast_ग्रन्थिः* nodep);
};
class AstVarScope final : सार्वजनिक Ast_ग्रन्थिः {
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
    bool m_fork_By_मानम् : 1;  // Set before a fork, whose processes get it by value
सार्वजनिक:
    AstVarScope(File_पङ्क्तिः* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_मानम् = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    शून्य cloneRelink() override {
        सचे (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त scopep()->name() + "->" + varp()->name(); }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override;
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    AstVar* varp() ध्रुव VL_MT_STABLE { पटिनिवत्त m_varp; }  // [After Link] Pointer to variable
    शून्य varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ध्रुव VL_MT_STABLE { पटिनिवत्त m_scopep; }  // Pointer to scope it's under
    शून्य scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ध्रुव { पटिनिवत्त m_trace; }
    शून्य trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ध्रुव { पटिनिवत्त m_optimizeLifePost; }
    शून्य optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_मानम्() ध्रुव { पटिनिवत्त m_fork_By_मानम्; }
    शून्य fork_By_मानम्(bool flag) { m_fork_By_मानम् = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : सार्वजनिक Ast_ग्रन्थिः_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ध्रुव uint64_t m_index;  // Index of the element, with the array's lo() at 0
सार्वजनिक:
    AstConsArrayItem(File_पङ्क्तिः* fl, uint64_t index, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_index{index} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    ध्रुव वर्ण* broken() ध्रुव override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        पटिनिवत्त nullptr;
    }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        पटिनिवत्त m_index == VN_DBG_AS(samep, ConsArrayItem)->m_index;
    }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }  // By AstConsArray's map
    uint64_t index() ध्रुव { पटिनिवत्त m_index; }
};
class AstConsAssocItem final : सार्वजनिक Ast_ग्रन्थिः_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
सार्वजनिक:
    AstConsAssocItem(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* keyp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    ध्रुव वर्ण* broken() ध्रुव override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        पटिनिवत्त nullptr;
    }
};
class AstConsDynArrayItem final : सार्वजनिक Ast_ग्रन्थिः_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
सार्वजनिक:
    AstConsDynArrayItem(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_पङ्क्तिः* fl, AstConst* keyp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    ध्रुव वर्ण* broken() ध्रुव override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        पटिनिवत्त nullptr;
    }
};
class AstConsQueueItem final : सार्वजनिक Ast_ग्रन्थिः_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
सार्वजनिक:
    AstConsQueueItem(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    ध्रुव वर्ण* broken() ध्रुव override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        पटिनिवत्त nullptr;
    }
};
class AstConsStructItem final : सार्वजनिक Ast_ग्रन्थिः_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
सार्वजनिक:
    AstConsStructItem(File_पङ्क्तिः* fl, AstMemberDType* memberp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    ध्रुव वर्ण* broken() ध्रुव override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        पटिनिवत्त nullptr;
    }
    शून्य dump(std::ostream& str) ध्रुव override;
    AstMemberDType* memberp() ध्रुव { पटिनिवत्त m_memberp; }
};
class AstConsWildcardItem final : सार्वजनिक Ast_ग्रन्थिः_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
सार्वजनिक:
    AstConsWildcardItem(File_पङ्क्तिः* fl, AstConst* keyp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    ध्रुव वर्ण* broken() ध्रुव override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        पटिनिवत्त nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : सार्वजनिक Ast_ग्रन्थिः_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    पूर्णाङ्क m_offset;  // Offset column numbers to uniq-ify IFs
सार्वजनिक:
    AstCoverOtherDecl(File_पङ्क्तिः* fl, ध्रुव string& page, ध्रुव string& comment,
                      ध्रुव string& linescov, पूर्णाङ्क offset, ध्रुव string& fsmVar = "",
                      ध्रुव string& fsmFrom = "", ध्रुव string& fsmTo = "",
                      ध्रुव string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क offset() ध्रुव { पटिनिवत्त m_offset; }
    पूर्णाङ्क size() ध्रुव override { पटिनिवत्त 1; }
    ध्रुव string& linescov() ध्रुव { पटिनिवत्त m_linescov; }
    ध्रुव string& fsmVar() ध्रुव { पटिनिवत्त m_fsmVar; }
    ध्रुव string& fsmFrom() ध्रुव { पटिनिवत्त m_fsmFrom; }
    ध्रुव string& fsmTo() ध्रुव { पटिनिवत्त m_fsmTo; }
    ध्रुव string& fsmTag() ध्रुव { पटिनिवत्त m_fsmTag; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstCoverOtherDecl* ध्रुव asamep = VN_DBG_AS(samep, CoverOtherDecl);
        पटिनिवत्त Super::same_ग्रन्थिः(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : सार्वजनिक Ast_ग्रन्थिः_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ध्रुव VNumRange m_range;  // Packed array range covering each toggle bit
सार्वजनिक:
    AstCoverToggleDecl(File_पङ्क्तिः* fl, ध्रुव string& page, ध्रुव string& comment,
                       ध्रुव VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    पूर्णाङ्क size() ध्रुव override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        पटिनिवत्त 2 * m_range.elements();
    }
    ध्रुव VNumRange& range() ध्रुव { पटिनिवत्त m_range; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstCoverToggleDecl* ध्रुव asamep = VN_DBG_AS(samep, CoverToggleDecl);
        पटिनिवत्त Super::same_ग्रन्थिः(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : सार्वजनिक Ast_ग्रन्थिः_FTask {
    // A function inside a module
सार्वजनिक:
    AstFunc(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* stmtp, Ast_ग्रन्थिः* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    Ast_ग्रन्थिः_FTask* cloneType(ध्रुव string& name) override {
        पटिनिवत्त new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ध्रुव override { पटिनिवत्त "function"; }
};
class AstLet final : सार्वजनिक Ast_ग्रन्थिः_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
सार्वजनिक:
    AstLet(File_पङ्क्तिः* fl, ध्रुव string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    ध्रुव वर्ण* broken() ध्रुव override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_व्यञ्जकम्));
        पटिनिवत्त nullptr;
    }
    Ast_ग्रन्थिः_FTask* cloneType(ध्रुव string& name) override { पटिनिवत्त new AstLet{fileline(), name}; }
    string verilogKwd() ध्रुव override { पटिनिवत्त "let"; }
};
class AstProperty final : सार्वजनिक Ast_ग्रन्थिः_FTask {
    // A property inside a module
सार्वजनिक:
    AstProperty(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    Ast_ग्रन्थिः_FTask* cloneType(ध्रुव string& name) override {
        पटिनिवत्त new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ध्रुव override { पटिनिवत्त "property"; }
};
class AstSequence final : सार्वजनिक Ast_ग्रन्थिः_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
सार्वजनिक:
    AstSequence(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool hasDType() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    Ast_ग्रन्थिः_FTask* cloneType(ध्रुव string& name) override {
        पटिनिवत्त new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ध्रुव override { पटिनिवत्त "sequence"; }
    bool isReferenced() ध्रुव { पटिनिवत्त m_referenced; }
    शून्य isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : सार्वजनिक Ast_ग्रन्थिः_FTask {
    // A task inside a module
सार्वजनिक:
    AstTask(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_ग्रन्थिः_FTask* cloneType(ध्रुव string& name) override {
        पटिनिवत्त new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ध्रुव override { पटिनिवत्त "task"; }
};

// === AstNodeFile ===
class AstCFile final : सार्वजनिक Ast_ग्रन्थिः_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
सार्वजनिक:
    AstCFile(File_पङ्क्तिः* fl, ध्रुव string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
    uint64_t complexityScore() ध्रुव { पटिनिवत्त m_complexityScore; }
    शून्य complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ध्रुव { पटिनिवत्त m_slow; }
    शून्य slow(bool flag) { m_slow = flag; }
    bool source() ध्रुव { पटिनिवत्त m_source; }
    शून्य source(bool flag) { m_source = flag; }
    bool support() ध्रुव { पटिनिवत्त m_support; }
    शून्य support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : सार्वजनिक Ast_ग्रन्थिः_File {
    // Verilog output file
    // Parents:  NETLIST
सार्वजनिक:
    AstVFile(File_पङ्क्तिः* fl, ध्रुव string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    शून्य dump(std::ostream& str = std::cout) ध्रुव override;
    शून्य dumpJson(std::ostream& str = std::cout) ध्रुव override;
};
class AstCoverCross final : सार्वजनिक Ast_ग्रन्थिः_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
सार्वजनिक:
    AstCoverCross(File_पङ्क्तिः* fl, ध्रुव string& name, AstCoverpointRef* itemsp,
                  Ast_ग्रन्थिः_व्यञ्जकम्* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstCoverpoint final : सार्वजनिक Ast_ग्रन्थिः_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
सार्वजनिक:
    AstCoverpoint(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः_व्यञ्जकम्* exprp,
                  Ast_ग्रन्थिः_व्यञ्जकम्* iffp = nullptr, Ast_ग्रन्थिः* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};

// === AstNodeGen ===
class AstGenBlock final : सार्वजनिक Ast_ग्रन्थिः_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ध्रुव bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ध्रुव bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

सार्वजनिक:
    AstGenBlock(File_पङ्क्तिः* fl, ध्रुव string& name, Ast_ग्रन्थिः* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    std::string name() ध्रुव override VL_MT_STABLE { पटिनिवत्त m_name; }
    शून्य name(ध्रुव std::string& name) override { m_name = name; }
    bool unnamed() ध्रुव { पटिनिवत्त m_unnamed; }
    bool implied() ध्रुव { पटिनिवत्त m_implied; }
    AstDefaultDisable* defaultDisablep() ध्रुव { पटिनिवत्त m_defaultDisablep; }
    शून्य defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ध्रुव string& outer) ध्रुव;
};
class AstGenCase final : सार्वजनिक Ast_ग्रन्थिः_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
सार्वजनिक:
    AstGenCase(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : सार्वजनिक Ast_ग्रन्थिः_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
सार्वजनिक:
    AstGenFor(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* initsp, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः* incsp, Ast_ग्रन्थिः* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : सार्वजनिक Ast_ग्रन्थिः_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
सार्वजनिक:
    AstGenIf(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* condp, Ast_ग्रन्थिः* thensp, Ast_ग्रन्थिः* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : सार्वजनिक Ast_ग्रन्थिः_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_अङ्कः;  // Declaration token number
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
    string dtypeNameCalc(bool full, ध्रुव string& scope) ध्रुव;
    string dtypeNameScope() ध्रुव;  // Prefix of dtypeName(true) for the scope declaring the class

सार्वजनिक:
    AstClass(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_अङ्कः{fl->token_अङ्कः()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ध्रुव override { पटिनिवत्त isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त true; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    bool timescaleMatters() ध्रुव override { पटिनिवत्त false; }
    AstClassPackage* classOrPackagep() ध्रुव VL_MT_STABLE { पटिनिवत्त m_classOrPackagep; }
    शून्य classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ध्रुव VL_MT_STABLE {
        पटिनिवत्त m_covergroupEnclosingClassp;
    }
    शून्य covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_ग्रन्थिः* membersp() ध्रुव VL_MT_STABLE { पटिनिवत्त stmtsp(); }
    शून्य addMembersp(Ast_ग्रन्थिः* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() ध्रुव { पटिनिवत्त m_hasRandVarsUpdate; }
    शून्य hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ध्रुव { पटिनिवत्त m_covergroup; }
    शून्य isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ध्रुव { पटिनिवत्त m_extended; }
    शून्य isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ध्रुव { पटिनिवत्त m_interfaceClass; }
    शून्य isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ध्रुव { पटिनिवत्त m_virtual; }
    शून्य isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ध्रुव { पटिनिवत्त m_needRNG; }
    शून्य needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() ध्रुव { पटिनिवत्त m_useVirtualPublic; }
    शून्य useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    शून्य markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ध्रुव { पटिनिवत्त m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) ध्रुव;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(ध्रुव string& scope) ध्रुव;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    शून्य dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ध्रुव { पटिनिवत्त !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_सन्देशः() ध्रुव override { पटिनिवत्त hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    स्थिर string embeddedCovergroupTypeName(ध्रुव string& name) { पटिनिवत्त "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ध्रुव;
    // Covergroup options accessors
    पूर्णाङ्क cgAutoBinMax() ध्रुव { पटिनिवत्त m_cgAutoBinMax; }
    शून्य cgAutoBinMax(पूर्णाङ्क मानम्) { m_cgAutoBinMax = मानम्; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    स्थिर bool isClassExtendedFrom(ध्रुव AstClass* refClassp, ध्रुव AstClass* baseClassp);
    uint32_t decl_Token_अङ्कः() ध्रुव override { पटिनिवत्त m_decl_Token_अङ्कः; }
    शून्य decl_Token_अङ्कः_Set_Min(uint32_t token_अङ्कः) override {
        m_decl_Token_अङ्कः = std::min(m_decl_Token_अङ्कः, token_अङ्कः);
    }
    शून्य baseOverride(ध्रुव VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ध्रुव { पटिनिवत्त m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    स्थिर bool isCacheableChild(ध्रुव Ast_ग्रन्थिः* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    template <typename T_Callable>
    शून्य foreachMember(ध्रुव T_Callable& f) {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_ग्रन्थिः*>::मानम्
                && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        सचे (ध्रुव AstClassExtends* ध्रुव cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        अत्थाय (Ast_ग्रन्थिः* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (Ast_ग्रन्थिः::privateTypeTest<T_ग्रन्थिः>(stmtp)) f(this, static_cast<T_ग्रन्थिः*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    template <typename T_Callable>
    bool existsMember(ध्रुव T_Callable& p) ध्रुव {
        using T_ग्रन्थिः = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ध्रुव AstClass*, ध्रुव T_ग्रन्थिः*>::मानम्
                && std::is_base_of<Ast_ग्रन्थिः, T_ग्रन्थिः>::मानम्,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        सचे (ध्रुव AstClassExtends* ध्रुव cextendsp = this->extendsp()) {
            सचे (cextendsp->classp()->existsMember(p)) पटिनिवत्त true;
        }
        अत्थाय (Ast_ग्रन्थिः* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            सचे (Ast_ग्रन्थिः::privateTypeTest<T_ग्रन्थिः>(stmtp)) {
                सचे (p(this, static_cast<T_ग्रन्थिः*>(stmtp))) पटिनिवत्त true;
            }
        }
        पटिनिवत्त false;
    }
};
class AstClassPackage final : सार्वजनिक Ast_ग्रन्थिः_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
सार्वजनिक:
    AstClassPackage(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ध्रुव override { पटिनिवत्त "classpackage"; }
    bool timescaleMatters() ध्रुव override { पटिनिवत्त false; }
    AstClass* classp() ध्रुव VL_MT_SAFE { पटिनिवत्त m_classp; }
    शून्य classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : सार्वजनिक Ast_ग्रन्थिः_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
सार्वजनिक:
    AstIface(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    string verilogKwd() ध्रुव override { पटिनिवत्त "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ध्रुव override { पटिनिवत्त false; }
    bool hasVirtualRef() ध्रुव { पटिनिवत्त m_hasVirtualRef; }
    शून्य setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) ध्रुव;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    शून्य dtypeNameFreeze();
};
class AstModule final : सार्वजनिक Ast_ग्रन्थिः_Module {
    // A module declaration
    ध्रुव bool m_isChecker;  // Module represents a checker
    ध्रुव bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

सार्वजनिक:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ध्रुव override {
        पटिनिवत्त m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() ध्रुव override { पटिनिवत्त true; }
    bool isChecker() ध्रुव { पटिनिवत्त m_isChecker; }
    bool isProgram() ध्रुव { पटिनिवत्त m_isProgram; }
    bool hasGenericIface() ध्रुव { पटिनिवत्त m_hasGenericIface; }
    शून्य hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
};
class AstNotFoundModule final : सार्वजनिक Ast_ग्रन्थिः_Module {
    // A missing module declaration
सार्वजनिक:
    AstNotFoundModule(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() ध्रुव override { पटिनिवत्त "/*not-found-*/ module"; }
    bool timescaleMatters() ध्रुव override { पटिनिवत्त false; }
};
class AstPackage final : सार्वजनिक Ast_ग्रन्थिः_Module {
    // A package declaration
सार्वजनिक:
    AstPackage(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ध्रुव override { पटिनिवत्त "package"; }
    bool timescaleMatters() ध्रुव override { पटिनिवत्त !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : सार्वजनिक Ast_ग्रन्थिः_Module {
    // A primitive declaration
सार्वजनिक:
    AstPrimitive(File_पङ्क्तिः* fl, ध्रुव string& name, ध्रुव string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ध्रुव override { पटिनिवत्त "primitive"; }
    bool timescaleMatters() ध्रुव override { पटिनिवत्त false; }
};

// === AstNodeProcedure ===
class AstAlways final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ध्रुव VAlwaysKwd m_keyword;

सार्वजनिक:
    AstAlways(File_पङ्क्तिः* fl, VAlwaysKwd keyword, Ast_Sen_वृक्षः* sentreep, Ast_ग्रन्थिः* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit अन्तःस्थ AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    VAlwaysKwd keyword() ध्रुव { पटिनिवत्त m_keyword; }
};
class AstAlwaysObserved final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

सार्वजनिक:
    AstAlwaysObserved(File_पङ्क्तिः* fl, Ast_Sen_वृक्षः* sentreep, Ast_ग्रन्थिः* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

सार्वजनिक:
    explicit AstAlwaysPost(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // Like always but Postponed scheduling region

सार्वजनिक:
    AstAlwaysPostponed(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

सार्वजनिक:
    explicit AstAlwaysPre(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

सार्वजनिक:
    AstAlwaysReactive(File_पङ्क्तिः* fl, Ast_Sen_वृक्षः* sentreep, Ast_ग्रन्थिः* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
सार्वजनिक:
    AstFinal(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
सार्वजनिक:
    AstInitial(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
सार्वजनिक:
    AstInitialAutomatic(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : सार्वजनिक Ast_ग्रन्थिः_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
सार्वजनिक:
    AstInitialStatic(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : सार्वजनिक Ast_ग्रन्थिः_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
सार्वजनिक:
    AstBracketRange(File_पङ्क्तिः* fl, Ast_ग्रन्थिः* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    आभास string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { पटिनिवत्त true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ध्रुव override VL_MT_SAFE { पटिनिवत्त false; }
};
class AstRange final : सार्वजनिक Ast_ग्रन्थिः_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ध्रुव bool m_fromBracket = false;  // From C-style '[x]' declaration
सार्वजनिक:
    AstRange(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_व्यञ्जकम्* leftp, Ast_ग्रन्थिः_व्यञ्जकम्* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    अन्तःस्थ AstRange(File_पङ्क्तिः* fl, पूर्णाङ्क वामः, पूर्णाङ्क दक्षिणः);
    अन्तःस्थ AstRange(File_पङ्क्तिः* fl, ध्रुव VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    अन्तःस्थ पूर्णाङ्क वामः_Const() ध्रुव VL_MT_STABLE;
    अन्तःस्थ पूर्णाङ्क दक्षिणः_Const() ध्रुव VL_MT_STABLE;
    पूर्णाङ्क hiConst() ध्रुव VL_MT_STABLE {
        ध्रुव पूर्णाङ्क l = वामः_Const();
        ध्रुव पूर्णाङ्क r = दक्षिणः_Const();
        पटिनिवत्त l > r ? l : r;
    }
    पूर्णाङ्क loConst() ध्रुव VL_MT_STABLE {
        ध्रुव पूर्णाङ्क l = वामः_Const();
        ध्रुव पूर्णाङ्क r = दक्षिणः_Const();
        पटिनिवत्त l > r ? r : l;
    }
    पूर्णाङ्क elementsConst() ध्रुव VL_MT_STABLE { पटिनिवत्त hiConst() - loConst() + 1; }
    bool ascending() ध्रुव { पटिनिवत्त वामः_Const() < दक्षिणः_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_वामः_To_दक्षिणः() ध्रुव {
        पटिनिवत्त VNumRange{वामः_Const(), दक्षिणः_Const()}.seq_वामः_To_दक्षिणः();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_दक्षिणः_To_वामः() ध्रुव {
        पटिनिवत्त VNumRange{वामः_Const(), दक्षिणः_Const()}.seq_दक्षिणः_To_वामः();
    }
    शून्य dump(std::ostream& str) ध्रुव override;
    शून्य dumpJson(std::ostream& str) ध्रुव override;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* samep) ध्रुव override {
        ध्रुव AstRange* ध्रुव asamep = VN_DBG_AS(samep, Range);
        पटिनिवत्त fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ध्रुव { पटिनिवत्त m_fromBracket; }
};
class AstUnsizedRange final : सार्वजनिक Ast_ग्रन्थिः_Range {
    // Unsized range specification, for open arrays
सार्वजनिक:
    explicit AstUnsizedRange(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    आभास string emitVerilog() { पटिनिवत्त "[]"; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { पटिनिवत्त true; }
};
class AstWildcardRange final : सार्वजनिक Ast_ग्रन्थिः_Range {
    // Wildcard range specification, for wildcard index type associative arrays
सार्वजनिक:
    explicit AstWildcardRange(File_पङ्क्तिः* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    आभास string emitC() { V3ERROR_NA_RETURN(""); }
    आभास string emitVerilog() { पटिनिवत्त "[*]"; }
    bool same_ग्रन्थिः(ध्रुव Ast_ग्रन्थिः* /*samep*/) ध्रुव override { पटिनिवत्त true; }
};

#endif  // Guard
