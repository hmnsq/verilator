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

#اگر_غیر_تعریف_شدہ VERILATOR_V3ASTNODES_H_
#تعریف_کرو VERILATOR_V3ASTNODES_H_

#اگر_غیر_تعریف_شدہ VERILATOR_V3AST_H_
#غلطی "Use V3Ast.h as the include"
#شامل "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کرو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#اگر_ختم

#شامل <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_گرہ_Cons_شے VL_NOT_FINAL : عوامی Ast_گرہ {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    دوست class Ast_گرہ_اظہاریہ;  // For m_knownConstantTree
    mutable bool m_معلوم_Constant_درخت = false;  // Known to be a tree of constants

محفوظ:
    Ast_گرہ_Cons_شے(VNType t, File_سطر* fl, Ast_گرہ_اظہاریہ* valuep)
        : Ast_گرہ{t, fl} {
        this->valuep(valuep);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Cons_شے;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class Ast_گرہ_Cover_Decl VL_NOT_FINAL : عوامی Ast_گرہ {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_صفحہ;  // Coverage point's page tag
    string m_متن;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    صحیح m_bin_عدد = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    صحیح m_local_Bin_عدد = 0;  // Per-module coverage bin offset
    bool m_فی_Instance = false;  // Don't clone during inlining, already per instance
عوامی:
    Ast_گرہ_Cover_Decl(VNType t, File_سطر* fl, ثابت string& صفحہ, ثابت string& comment)
        : Ast_گرہ(t, fl)
        , m_صفحہ{صفحہ}
        , m_متن{comment} {}
    ASTGEN_MEMBERS_Ast_گرہ_Cover_Decl;
    ثابت حرف* broken() ثابت override {
        اگر (m_ڈیٹا_Declp
            && (m_ڈیٹا_Declp == this || m_ڈیٹا_Declp->m_ڈیٹا_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvt_مقصد_Hex(this));
        }
        واپس nullptr;
    }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح instr_گنتی() ثابت override { واپس 1 + 2 * INSTR_گنتی_LD; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    صحیح bin_عدد() ثابت { واپس m_bin_عدد; }
    خالی bin_عدد(صحیح flag) { m_bin_عدد = flag; }
    صحیح local_Bin_عدد() ثابت { واپس m_local_Bin_عدد; }
    خالی local_Bin_عدد(صحیح flag) { m_local_Bin_عدد = flag; }
    bool فی_Instance() ثابت { واپس m_فی_Instance; }
    خالی فی_Instance(bool flag) { m_فی_Instance = flag; }
    مجازی صحیح سائز() ثابت = 0;
    ثابت string& comment() ثابت { واپس m_متن; }  // text to insert in code
    ثابت string& صفحہ() ثابت { واپس m_صفحہ; }
    ثابت string& hier() ثابت { واپس m_hier; }
    خالی hier(ثابت string& flag) { m_hier = flag; }
    خالی comment(ثابت string& flag) { m_متن = flag; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت Ast_گرہ_Cover_Decl* ثابت asamep = VN_DBG_AS(samep, گرہ_Cover_Decl);
        واپس (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && صفحہ() == asamep->صفحہ()
                && فی_Instance() == asamep->فی_Instance());
    }
    bool isPredictOptimizable() ثابت override { واپس false; }
    خالی ڈیٹا_Declp(Ast_گرہ_Cover_Decl* nodep) { m_ڈیٹا_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_گرہ_Cover_Decl* ڈیٹا_Decl_Nullp() ثابت { واپس m_ڈیٹا_Declp; }
    Ast_گرہ_Cover_Decl* ڈیٹا_Decl_Thisp() { واپس ڈیٹا_Decl_Nullp() ? ڈیٹا_Decl_Nullp() : this; }
};
class Ast_گرہ_FTask VL_NOT_FINAL : عوامی Ast_گرہ {
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
    string m_iface_پورٹ_Name;  // Interface port name for out-of-block definition (IEEE 25.8)
    uint64_t m_dpi_Open_والد = 0;  // DPI import open array, if !=0, how many callees
    bool m_task_عوامی : 1;  // Public task
    bool m_class_طریقہ : 1;  // Class method
    bool m_didProto : 1;  // Did prototype processing
    bool m_prototype : 1;  // Just a prototype
    bool m_dpiExport : 1;  // DPI exported
    bool m_dpiImport : 1;  // DPI imported
    bool m_dpiContext : 1;  // DPI import context
    bool m_dpi_Open_بچہ : 1;  // DPI import open array child wrapper
    bool m_dpiTask : 1;  // DPI import task (vs. void function)
    bool m_isConstructor : 1;  // Class constructor
    bool m_isExternProto : 1;  // Extern prototype
    bool m_isExternDef : 1;  // Extern definition
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_dpi_خالص : 1;  // DPI import pure (vs. virtual pure)
    bool m_keepAlive : 1;  // Disable dead function elimination
    bool m_needsSyms : 1;  // Requires vlSymsp argument
    bool m_خالص_Virtual : 1;  // Pure virtual
    bool m_recursive : 1;  // Recursive or part of recursion
    bool m_static : 1;  // Static method in class
    bool m_underGenerate : 1;  // Under generate (for warning)
    bool m_verilogFunction : 1;  // Declared by user as function (versus internal-made)
    bool m_verilogTask : 1;  // Declared by user as task (versus internal-made)
    bool m_virtual : 1;  // Virtual method in class
    bool m_needProcess : 1;  // Needs access to VlProcess of the caller
    bool m_isCovergroupSample : 1;  // Covergroup sample() method
    VBaseOverride m_بنیاد_Override;  // BaseOverride (inital/final/extends)
    VLifetime m_lifetime;  // Default lifetime of local vars
    VIsCached m_purity;  // Pure state

محفوظ:
    Ast_گرہ_FTask(VNType t, File_سطر* fl, ثابت string& name, Ast_گرہ* stmtsp)
        : Ast_گرہ{t, fl}
        , m_name{name}
        , m_task_عوامی{false}
        , m_class_طریقہ{false}
        , m_didProto{false}
        , m_prototype{false}
        , m_dpiExport{false}
        , m_dpiImport{false}
        , m_dpiContext{false}
        , m_dpi_Open_بچہ{false}
        , m_dpiTask{false}
        , m_isConstructor{false}
        , m_isExternProto{false}
        , m_isExternDef{false}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_dpi_خالص{false}
        , m_keepAlive{false}
        , m_needsSyms{true}
        , m_خالص_Virtual{false}
        , m_recursive{false}
        , m_static{false}
        , m_underGenerate{false}
        , m_verilogFunction{false}
        , m_verilogTask{false}
        , m_virtual{false}
        , m_needProcess{false}
        , m_isCovergroupSample{false} {
        جوڑو_Stmtsp(stmtsp);
        cname(name);  // Might be overridden by dpi import/export
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_FTask;
    مجازی Ast_گرہ_FTask* cloneType(ثابت string& name) = 0;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool isGateOptimizable() ثابت override {
        واپس !((m_dpiExport || m_dpiImport) && !m_dpi_خالص);
    }
    // {AstFunc only} op1 = Range output variable
    خالی name(ثابت string& name) override { m_name = name; }
    string cname() ثابت { واپس m_cname; }
    خالی cname(ثابت string& cname) { m_cname = cname; }
    bool isFunction() ثابت { واپس fvarp() != nullptr; }
    // MORE ACCESSORS
    خالی dpi_Open_والد_Inc() { ++m_dpi_Open_والد; }
    خالی dpi_Open_والد_Clear() { m_dpi_Open_والد = 0; }
    uint64_t dpi_Open_والد() ثابت { واپس m_dpi_Open_والد; }
    bool task_عوامی() ثابت { واپس m_task_عوامی; }
    خالی task_عوامی(bool flag) { m_task_عوامی = flag; }
    bool class_طریقہ() ثابت { واپس m_class_طریقہ; }
    خالی class_طریقہ(bool flag) { m_class_طریقہ = flag; }
    bool didProto() ثابت { واپس m_didProto; }
    خالی didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ثابت { واپس m_isExternProto; }
    خالی isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ثابت { واپس m_isExternDef; }
    خالی isExternDef(bool flag) { m_isExternDef = flag; }
    ثابت string& iface_پورٹ_Name() ثابت { واپس m_iface_پورٹ_Name; }
    خالی iface_پورٹ_Name(ثابت string& name) { m_iface_پورٹ_Name = name; }
    bool prototype() ثابت { واپس m_prototype; }
    خالی prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ثابت { واپس m_dpiExport; }
    خالی dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ثابت { واپس m_dpiImport; }
    خالی dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ثابت { واپس m_dpiContext; }
    خالی dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpi_Open_بچہ() ثابت { واپس m_dpi_Open_بچہ; }
    خالی dpi_Open_بچہ(bool flag) { m_dpi_Open_بچہ = flag; }
    bool dpiTask() ثابت { واپس m_dpiTask; }
    خالی dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ثابت { واپس !m_dpiCDecl.خالی_خانے(); }
    ثابت string& dpiCDecl() ثابت { واپس m_dpiCDecl; }
    خالی dpiCDecl(ثابت string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ثابت { واپس m_isConstructor; }
    خالی isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ثابت { واپس m_isHideLocal; }
    خالی isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ثابت { واپس m_isHideProtected; }
    خالی isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpi_خالص() ثابت { واپس m_dpi_خالص; }
    خالی dpi_خالص(bool flag) { m_dpi_خالص = flag; }
    bool keepAlive() ثابت { واپس m_keepAlive; }
    خالی keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ثابت { واپس m_needsSyms; }
    خالی needsSyms(bool flag) { m_needsSyms = flag; }
    bool خالص_Virtual() ثابت { واپس m_خالص_Virtual; }
    خالی خالص_Virtual(bool flag) { m_خالص_Virtual = flag; }
    bool recursive() ثابت { واپس m_recursive; }
    خالی recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ثابت { واپس m_static; }
    خالی isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ثابت { واپس m_underGenerate; }
    خالی underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ثابت { واپس m_verilogFunction; }
    خالی verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ثابت { واپس m_verilogTask; }
    خالی verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ثابت { واپس m_virtual; }
    خالی isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ثابت { واپس m_needProcess; }
    خالی سیٹ_Need_Process() { m_needProcess = true; }
    bool isCovergroupSample() ثابت { واپس m_isCovergroupSample; }
    خالی isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    خالی بنیاد_Override(ثابت VBaseOverride& flag) { m_بنیاد_Override = flag; }
    VBaseOverride بنیاد_Override() ثابت { واپس m_بنیاد_Override; }
    خالی lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ثابت { واپس m_lifetime; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;
    خالی propagateAttrFrom(ثابت Ast_گرہ_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        class_طریقہ(fromp->class_طریقہ());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

نجی:
    bool getPurityRecurse() ثابت;
};
class Ast_گرہ_File VL_NOT_FINAL : عوامی Ast_گرہ {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
عوامی:
    Ast_گرہ_File(VNType t, File_سطر* fl, ثابت string& name)
        : Ast_گرہ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_گرہ_File;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_گرہ_Func_Cov_شے VL_NOT_FINAL : عوامی Ast_گرہ {
    // Base class for functional coverage items (coverpoints, crosses)
محفوظ:
    string m_name;  // Item name

عوامی:
    Ast_گرہ_Func_Cov_شے(VNType t, File_سطر* fl, ثابت string& name)
        : Ast_گرہ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_گرہ_Func_Cov_شے;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& name) override { m_name = name; }
    bool maybe_Pointed_مقصد() ثابت override { واپس true; }
};
class Ast_گرہ_Gen VL_NOT_FINAL : عوامی Ast_گرہ {
    // Generate construct
عوامی:
    Ast_گرہ_Gen(VNType t, File_سطر* fl)
        : Ast_گرہ{t, fl} {}
    ASTGEN_MEMBERS_Ast_گرہ_Gen;
};
class Ast_گرہ_Module VL_NOT_FINAL : عوامی Ast_گرہ {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    ثابت string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    صحیح m_گہرائی = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    صحیح m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
    VLifetime m_lifetime;  // Lifetime
    VTimescale m_timeunit;  // Global time unit
    VOptionBool m_unconnectedDrive;  // State of `unconnected_drive
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

    bool m_mod_عوامی : 1;  // Module has public references
    bool m_modTrace : 1;  // Tracing this module
    bool m_اندر_Library : 1;  // From a library, no error if not used, never top level
    bool m_ctorVarReset : 1;  // Ctor needs to call ctor_var_reset
    bool m_dead : 1;  // LinkDot believes is dead; will remove in Dead visitors
    bool m_hasGParam : 1;  // Has global parameter (for link)
    bool m_hasParameterList : 1;  // Has #() for parameter declaration
    bool m_hier_بلاک : 1;  // Hierarchical Block marked by HIER_BLOCK pragma
    bool m_hierParams : 1;  // Block containing params for parameterized hier blocks
    bool m_recursive : 1;  // Recursive module
    bool m_recursiveClone : 1;  // If recursive, what module it clones, otherwise nullptr
    bool m_parameterizedTemplate : 1;  // True when at least one specialized clone exists;
                                       // set by V3Param::deepCloneModule. Suppresses
                                       // width/type errors on the unresolved template.
    bool m_verilatorLib : 1;  // Module is a stub for a Verilator produced --lib-create
محفوظ:
    Ast_گرہ_Module(VNType t, File_سطر* fl, ثابت string& name, ثابت string& libname)
        : Ast_گرہ{t, fl}
        , m_name{name}
        , m_origName{name}
        , m_libname{libname}
        , m_mod_عوامی{false}
        , m_modTrace{false}
        , m_اندر_Library{false}
        , m_ctorVarReset{false}
        , m_dead{false}
        , m_hasGParam{false}
        , m_hasParameterList{false}
        , m_hier_بلاک{false}
        , m_hierParams{false}
        , m_recursive{false}
        , m_recursiveClone{false}
        , m_parameterizedTemplate{false}
        , m_verilatorLib{false} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Module;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    مجازی bool timescaleMatters() ثابت = 0;
    اندرونی bool isDollarUnit() ثابت;  // Is the $unit package
    اندرونی bool isConstPool() ثابت;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ثابت string& outer) ثابت;
    // ACCESSORS
    خالی name(ثابت string& name) override { m_name = name; }
    string origName() ثابت override { واپس m_origName; }
    string someInstanceName() ثابت VL_MT_محفوظ { واپس m_someInstanceName; }
    خالی someInstanceName(ثابت string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ثابت { واپس m_defaultDisablep; }
    خالی defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool اندر_Library() ثابت { واپس m_اندر_Library; }
    خالی اندر_Library(bool flag) { m_اندر_Library = flag; }
    خالی گہرائی(صحیح قدر) { m_گہرائی = قدر; }
    صحیح گہرائی() ثابت VL_MT_محفوظ { واپس m_گہرائی; }
    خالی level(صحیح قدر) { m_level = قدر; }
    صحیح level() ثابت VL_MT_محفوظ { واپس m_level; }
    string libname() ثابت { واپس m_libname; }
    bool is_چوٹی() ثابت VL_MT_محفوظ { واپس level() == 1; }
    bool mod_عوامی() ثابت { واپس m_mod_عوامی; }
    خالی mod_عوامی(bool flag) { m_mod_عوامی = flag; }
    bool modTrace() ثابت { واپس m_modTrace; }
    خالی modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ثابت { واپس m_ctorVarReset; }
    خالی ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ثابت { واپس m_dead; }
    خالی dead(bool flag) { m_dead = flag; }
    bool hasGParam() ثابت { واپس m_hasGParam; }
    خالی hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ثابت { واپس m_hasParameterList; }
    خالی hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hier_بلاک() ثابت { واپس m_hier_بلاک; }
    خالی hier_بلاک(bool flag) { m_hier_بلاک = flag; }
    bool hierParams() ثابت { واپس m_hierParams; }
    خالی hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ثابت { واپس m_recursive; }
    خالی recursive(bool flag) { m_recursive = flag; }
    خالی recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ثابت { واپس m_recursiveClone; }
    bool parameterizedTemplate() ثابت { واپس m_parameterizedTemplate; }
    خالی parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    خالی verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ثابت { واپس m_verilatorLib; }
    VLifetime lifetime() ثابت { واپس m_lifetime; }
    خالی lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ثابت { واپس m_unconnectedDrive; }
    خالی unconnectedDrive(ثابت VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_گرہ_Procedure VL_NOT_FINAL : عوامی Ast_گرہ {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
محفوظ:
    Ast_گرہ_Procedure(VNType t, File_سطر* fl, Ast_گرہ* stmtsp)
        : Ast_گرہ{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        جوڑو_Stmtsp(stmtsp);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Procedure;
    // METHODS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isJustOneBodyStmt() ثابت { واپس stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ثابت { واپس m_suspendable; }
    خالی سیٹ_Suspendable() { m_suspendable = true; }
    bool needProcess() ثابت { واپس m_needProcess; }
    خالی سیٹ_Need_Process() { m_needProcess = true; }
};
class Ast_گرہ_Range VL_NOT_FINAL : عوامی Ast_گرہ {
    // A range, sized or unsized
محفوظ:
    Ast_گرہ_Range(VNType t, File_سطر* fl)
        : Ast_گرہ{t, fl} {}

عوامی:
    ASTGEN_MEMBERS_Ast_گرہ_Range;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_فعال final : عوامی Ast_گرہ {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

عوامی:
    Ast_فعال(File_سطر* fl, ثابت string& name, Ast_Sen_درخت* sentreep)
        : ASTGEN_SUPER_فعال(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_Ast_فعال;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    // Statements are broken into pieces, as some must come before others.
    خالی sentreep(Ast_Sen_درخت* nodep) { m_sentreep = nodep; }
    Ast_Sen_درخت* sentreep() ثابت { واپس m_sentreep; }
    // METHODS
    اندرونی bool hasClocked() ثابت;
    اندرونی bool hasCombo() ثابت;
    اندرونی bool hasInitial() ثابت;
    اندرونی bool hasStatic() ثابت;
};
class AstAlias final : عوامی Ast_گرہ {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
عوامی:
    AstAlias(File_سطر* fl, Ast_گرہ_اظہاریہ* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : عوامی Ast_گرہ {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
عوامی:
    AstAliasScope(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : عوامی Ast_گرہ {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
عوامی:
    AstBind(File_سطر* fl, ثابت string& name, Ast_گرہ* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        جوڑو_Cellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Bind Target name
    خالی name(ثابت string& name) override { m_name = name; }
};
class AstCFunc final : عوامی Ast_گرہ {
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
    bool m_decl_نجی : 1;  // Declare it private
    bool m_keep_If_خالی_خانے : 1;  // Keep declaration and definition separate, even if empty
    bool m_slow : 1;  // Slow routine, called once or just at init time
    bool m_func_عوامی : 1;  // From user public task/function
    bool m_isConstructor : 1;  // Is C class constructor
    bool m_isDestructor : 1;  // Is C class destructor
    bool m_is_طریقہ : 1;  // Is inside a class definition
    bool m_isLoose : 1;  // Semantically this is a method, but is implemented as a function with
                         // an explicitly passed 'self' pointer as the first argument.  This can
                         // be slightly faster due to __restrict, and we do not declare in header
                         // so adding/removing loose functions doesn't recompile everything.
    bool m_isVirtual : 1;  // Virtual function
    bool m_اندراج_نقطہ : 1;  // User may call into this top level function
    bool m_dpi_خالص : 1;  // Pure DPI function
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
    صحیح m_لاگت;  // Function call cost
عوامی:
    AstCFunc(File_سطر* fl, ثابت string& name, AstScope* scopep, ثابت string& rtnType = "")
        : ASTGEN_SUPER_CFunc(fl) {
        m_isConst = VBoolOrUnknown::BU_UNKNOWN;  // Unknown until analyzed
        m_scopep = scopep;
        m_name = name;
        m_rtnType = rtnType;
        m_isStatic = false;
        m_isTrace = false;
        m_dontCombine = false;
        m_decl_نجی = false;
        m_keep_If_خالی_خانے = false;
        m_slow = false;
        m_func_عوامی = false;
        m_isConstructor = false;
        m_isDestructor = false;
        m_is_طریقہ = true;
        m_isLoose = false;
        m_isVirtual = false;
        m_needProcess = false;
        m_اندراج_نقطہ = false;
        m_dpi_خالص = false;
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
        m_لاگت = v3Global.opt.instr_گنتی_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstCFunc* ثابت asamep = VN_DBG_AS(samep, CFunc);
        واپس ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    خالی name(ثابت string& name) override { m_name = name; }
    صحیح instr_گنتی() ثابت override { واپس m_لاگت; }
    VBoolOrUnknown isConst() ثابت { واپس m_isConst; }
    خالی isConst(bool flag) { m_isConst.سیٹ_True_Or_False(flag); }
    خالی isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ثابت { واپس m_isStatic; }
    خالی isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ثابت VL_MT_محفوظ { واپس m_isTrace; }
    خالی isTrace(bool flag) { m_isTrace = flag; }
    خالی cname(ثابت string& name) { m_cname = name; }
    string cname() ثابت { واپس m_cname; }
    AstScope* scopep() ثابت { واپس m_scopep; }
    خالی scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ثابت { واپس ((m_rtnType == "") ? "void" : m_rtnType); }
    خالی rtnType(ثابت string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ثابت { واپس m_dontCombine || isTrace() || اندراج_نقطہ(); }
    خالی dontCombine(bool flag) { m_dontCombine = flag; }
    bool decl_نجی() ثابت { واپس m_decl_نجی; }
    خالی decl_نجی(bool flag) { m_decl_نجی = flag; }
    bool keep_If_خالی_خانے() ثابت VL_MT_محفوظ { واپس m_keep_If_خالی_خانے; }
    خالی keep_If_خالی_خانے(bool flag) { m_keep_If_خالی_خانے = flag; }
    bool slow() ثابت VL_MT_محفوظ { واپس m_slow; }
    خالی slow(bool flag) { m_slow = flag; }
    bool func_عوامی() ثابت { واپس m_func_عوامی; }
    خالی func_عوامی(bool flag) { m_func_عوامی = flag; }
    خالی argTypes(ثابت string& str) { m_argTypes = str; }
    string argTypes() ثابت { واپس m_argTypes; }
    خالی ifdef(ثابت string& str) { m_ifdef = str; }
    string ifdef() ثابت { واپس m_ifdef; }
    bool isConstructor() ثابت { واپس m_isConstructor; }
    خالی isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ثابت { واپس m_isDestructor; }
    خالی isDestructor(bool flag) { m_isDestructor = flag; }
    bool is_طریقہ() ثابت { واپس m_is_طریقہ; }
    خالی is_طریقہ(bool flag) { m_is_طریقہ = flag; }
    bool isLoose() ثابت { واپس m_isLoose; }
    خالی isLoose(bool flag) { m_isLoose = flag; }
    bool is_Proper_طریقہ() ثابت { واپس is_طریقہ() && !isLoose(); }
    bool isVirtual() ثابت { واپس m_isVirtual; }
    خالی isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ثابت { واپس m_needProcess; }
    خالی سیٹ_Need_Process() { m_needProcess = true; }
    bool اندراج_نقطہ() ثابت { واپس m_اندراج_نقطہ; }
    خالی اندراج_نقطہ(bool flag) { m_اندراج_نقطہ = flag; }
    bool dpi_خالص() ثابت { واپس m_dpi_خالص; }
    خالی dpi_خالص(bool flag) { m_dpi_خالص = flag; }
    bool dpiContext() ثابت { واپس m_dpiContext; }
    خالی dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ثابت VL_MT_محفوظ { واپس m_dpiExportDispatcher; }
    خالی dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ثابت { واپس m_dpiExportImpl; }
    خالی dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ثابت VL_MT_محفوظ { واپس m_dpiImportPrototype; }
    خالی dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ثابت { واپس m_dpiImportWrapper; }
    خالی dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ثابت { واپس !m_dpiCDecl.خالی_خانے(); }
    ثابت string& dpiCDecl() ثابت { واپس m_dpiCDecl; }
    خالی dpiCDecl(ثابت string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ثابت { واپس m_rtnType == "VlCoroutine"; }
    خالی recursive(bool flag) { m_recursive = flag; }
    bool recursive() ثابت { واپس m_recursive; }
    خالی unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ثابت override { واپس m_unlikely; }  // Note virtual override
    خالی noLife(bool flag) { m_noLife = flag; }
    bool noLife() ثابت { واپس m_noLife; }
    bool isCovergroupSample() ثابت { واپس m_isCovergroupSample; }
    خالی isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ثابت { واپس m_voidSelf; }
    خالی voidSelf(bool flag) { m_voidSelf = flag; }
    خالی لاگت(صحیح لاگت) { m_لاگت = لاگت; }
    // Special methods
    bool خالی_خانے_Body() ثابت {
        واپس !keep_If_خالی_خانے() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : عوامی Ast_گرہ {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ثابت string m_name;
    ثابت VUseType m_useType;  // What sort of use this is

عوامی:
    AstCUse(File_سطر* fl, VUseType useType, ثابت string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    VUseType useType() ثابت { واپس m_useType; }
};
class AstCell final : عوامی Ast_گرہ {
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
    File_سطر* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    صحیح m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
عوامی:
    AstCell(File_سطر* fl, File_سطر* mfl, ثابت string& instName, ثابت string& modName,
            AstPin* pinsp, AstPin* paramsp, AstRange* rangep)
        : ASTGEN_SUPER_Cell(fl)
        , m_modNameFileline{mfl}
        , m_name{instName}
        , m_origName{instName}
        , m_modName{modName}
        , m_hasIfaceVar{false}
        , m_recursive{false}
        , m_trace{true} {
        جوڑو_Pinsp(pinsp);
        جوڑو_Paramsp(paramsp);
        جوڑو_Rangep(rangep);
    }
    ASTGEN_MEMBERS_AstCell;
    // No cloneRelink, we presume cloneee's want the same module linkages
    خالی cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Cell name
    خالی name(ثابت string& name) override { m_name = name; }
    string origName() ثابت override { واپس m_origName; }  // * = Original name
    خالی origName(ثابت string& name) { m_origName = name; }
    string modName() ثابت { واپس m_modName; }  // * = Instance name
    خالی modName(ثابت string& name) { m_modName = name; }
    File_سطر* modNameFileline() ثابت { واپس m_modNameFileline; }
    Ast_گرہ_Module* modp() ثابت { واپس m_modp; }  // [AfterLink] = Pointer to module instantiated
    خالی modp(Ast_گرہ_Module* nodep) { m_modp = nodep; }
    صحیح arrayIdx() ثابت { واپس m_arrayIdx; }
    خالی arrayIdx(صحیح idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ثابت { واپس m_hasIfaceVar; }
    خالی hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    خالی trace(bool flag) { m_trace = flag; }
    bool isTrace() ثابت { واپس m_trace; }
    خالی recursive(bool flag) { m_recursive = flag; }
    bool recursive() ثابت { واپس m_recursive; }
};
class AstCellInline final : عوامی Ast_گرہ {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ثابت std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ثابت std::string m_origModName;  // __BEGIN__ or __FORK__
عوامی:
    AstCellInline(File_سطر* fl, ثابت std::string& name, ثابت std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    // ACCESSORS
    std::string name() ثابت override VL_MT_STABLE { واپس m_name; }
    std::string origModName() ثابت { واپس m_origModName; }
};
class AstCellInlineScope final : عوامی Ast_گرہ {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ثابت std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
عوامی:
    AstCellInlineScope(File_سطر* fl, ثابت std::string& name, ثابت std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    // ACCESSORS
    std::string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت std::string& name) override { m_name = name; }
    std::string origModName() ثابت { واپس m_origModName; }
};
class Ast_Cg_Option_تفویض final : عوامی Ast_گرہ {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ثابت VCoverOptionType m_optType;  // Option type
    ثابت string m_name;  // Original option name (for diagnostics on unknown options)
    ثابت bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
عوامی:
    Ast_Cg_Option_تفویض(File_سطر* fl, bool typeOption, VCoverOptionType optType,
                      ثابت string& rawName, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cg_Option_تفویض(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Cg_Option_تفویض;
    // ACCESSORS
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    VCoverOptionType optType() ثابت { واپس m_optType; }
    bool typeOption() ثابت { واپس m_typeOption; }
};
class AstClassExtends final : عوامی Ast_گرہ {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ثابت bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

عوامی:
    AstClassExtends(File_سطر* fl, Ast_گرہ* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    string verilogKwd() ثابت override { واپس isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ثابت;
    AstClass* classp() ثابت;  // Like above, but throws error if nulll
    bool isImplements() ثابت { واپس m_isImplements; }
    خالی parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ثابت { واپس m_parameterized; }
};
class AstClocking final : عوامی Ast_گرہ {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ثابت bool m_isGlobal;  // True if global clocking

عوامی:
    AstClocking(File_سطر* fl, ثابت std::string& name, Ast_Sen_درخت* sentreep, Ast_گرہ* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    std::string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool isDefault() ثابت { واپس m_isDefault; }
    bool isGlobal() ثابت { واپس m_isGlobal; }
    AstVar* ensureEventp(bool بچہ_DType = false);
    خالی makeDefault() { m_isDefault = true; }
};
class Ast_Clocking_شے final : عوامی Ast_گرہ {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

عوامی:
    Ast_Clocking_شے(File_سطر* fl, VDirection direction, Ast_گرہ_اظہاریہ* skewp, Ast_گرہ* clockingDeclp)
        : ASTGEN_SUPER_Clocking_شے(fl) {
        m_direction = direction;
        this->skewp(skewp);
        اگر (Ast_تفویض* ثابت clkAssignp = VN_CAST(clockingDeclp, تفویض)) {
            this->assignp(clkAssignp);
        } ورنہ {
            exprp(VN_AS(clockingDeclp, گرہ_اظہاریہ));
        }
    }
    ASTGEN_MEMBERS_Ast_Clocking_شے;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VDirection direction() ثابت { واپس m_direction; }
    Ast_Clocking_شے* outputp() ثابت { واپس m_outputp; }
    خالی outputp(Ast_Clocking_شے* outputp) { m_outputp = outputp; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
};
class AstConfig final : عوامی Ast_گرہ {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

عوامی:
    AstConfig(File_سطر* fl, ثابت std::string& libname, ثابت std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    std::string name() ثابت override VL_MT_STABLE { واپس m_libname + "." + m_configname; }
    std::string libname() ثابت VL_MT_STABLE { واپس m_libname; }
    std::string configname() ثابت VL_MT_STABLE { واپس m_configname; }
};
class AstConfigCell final : عوامی Ast_گرہ {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

عوامی:
    AstConfigCell(File_سطر* fl, ثابت std::string& libname, ثابت std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ثابت override VL_MT_STABLE { واپس m_libname + "." + m_cellname; }
    std::string libname() ثابت VL_MT_STABLE { واپس m_libname; }
    std::string cellname() ثابت VL_MT_STABLE { واپس m_cellname; }
};
class Ast_Config_قاعدہ final : عوامی Ast_گرہ {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ثابت bool m_isCell;  // Declared as "cell" versus "instance"

عوامی:
    Ast_Config_قاعدہ(File_سطر* fl, Ast_گرہ* cellp, Ast_گرہ* usep, bool isCell)
        : ASTGEN_SUPER_Config_قاعدہ(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        جوڑو_Usep(usep);
    }
    ASTGEN_MEMBERS_Ast_Config_قاعدہ;
    bool isCell() ثابت VL_MT_STABLE { واپس m_isCell; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class AstConfigUse final : عوامی Ast_گرہ {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ثابت bool m_isConfig;  // ":config"; Config, not module/primitive name

عوامی:
    AstConfigUse(File_سطر* fl, ثابت std::string& libname, ثابت std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        جوڑو_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ثابت override VL_MT_STABLE { واپس m_libname + "." + m_cellname; }
    std::string libname() ثابت VL_MT_STABLE { واپس m_libname; }
    std::string cellname() ثابت VL_MT_STABLE { واپس m_cellname; }
    bool isConfig() ثابت VL_MT_STABLE { واپس m_isConfig; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class AstConstraint final : عوامی Ast_گرہ {
    // Constraint
    // @astgen op1 := itemsp : List[AstNode]
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    string m_name;  // Name of constraint
    VBaseOverride m_بنیاد_Override;  // BaseOverride (inital/final/extends)
    bool m_isExternDef = false;  // Extern prototype definition
    bool m_isExternExplicit = false;  // Explicit prototype declaration (has extern)
    bool m_isExternProto = false;  // Prototype declaration (implicit or explicit)
    bool m_is_Kwd_خالص = false;  // Pure constraint
    bool m_isStatic = false;  // Static constraint
عوامی:
    AstConstraint(File_سطر* fl, ثابت string& name, Ast_گرہ* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Scope name
    خالی name(ثابت string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    خالی بنیاد_Override(ثابت VBaseOverride& flag) { m_بنیاد_Override = flag; }
    VBaseOverride بنیاد_Override() ثابت { واپس m_بنیاد_Override; }
    bool isExternDef() ثابت { واپس m_isExternDef; }
    خالی isExternDef(bool flag) { m_isExternDef = flag; }
    خالی isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ثابت { واپس m_isExternExplicit; }
    خالی isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ثابت { واپس m_isExternProto; }
    خالی is_Kwd_خالص(bool flag) { m_is_Kwd_خالص = flag; }
    bool is_Kwd_خالص() ثابت { واپس m_is_Kwd_خالص; }
    خالی isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ثابت { واپس m_isStatic; }
};
class AstCoverBin final : عوامی Ast_گرہ {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    ثابت string m_name;  // Base name of the bin
    ثابت VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

عوامی:
    AstCoverBin(File_سطر* fl, ثابت string& name, Ast_گرہ* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_گرہ_اظہاریہ* iffp = nullptr)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{isIllegal ? VCoverBinsType::BINS_ILLEGAL
                               : (isIgnore ? VCoverBinsType::BINS_IGNORE
                                           : (isWildcard ? VCoverBinsType::BINS_WILDCARD
                                                         : VCoverBinsType::BINS_USER))}
        , m_isWildcard{isWildcard} {
        جوڑو_Rangesp(rangesp);
        this->iffp(iffp);
    }
    // Constructor for automatic bins
    AstCoverBin(File_سطر* fl, ثابت string& name, Ast_گرہ_اظہاریہ* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_سطر* fl, ثابت string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_سطر* fl, ثابت string& name, Ast_Cover_Trans_سیٹ* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        جوڑو_Transp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    VCoverBinsType binsType() ثابت { واپس m_binsType; }
    bool isWildcard() ثابت { واپس m_isWildcard; }
    bool isArray() ثابت { واپس m_isArray; }
    خالی isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : عوامی Ast_گرہ {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ثابت bool m_isNegated;  // Complement the selection within the cross product

عوامی:
    AstCoverBinsof(File_سطر* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_گرہ* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        جوڑو_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& name) override { m_name = name; }
    bool isNegated() ثابت { واپس m_isNegated; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverBinsof* ثابت asamep = VN_DBG_AS(samep, CoverBinsof);
        واپس m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : عوامی Ast_گرہ {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ثابت string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ثابت VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

عوامی:
    AstCoverCrossBin(File_سطر* fl, ثابت string& name, Ast_گرہ* selectp, Ast_گرہ_اظہاریہ* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    string verilogKwd() ثابت override;
    VCoverBinsType binsType() ثابت { واپس m_binsType; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverCrossBin* ثابت asamep = VN_DBG_AS(samep, CoverCrossBin);
        واپس m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : عوامی Ast_گرہ {
    // The complete bin-tuple space of the enclosing cross
    ثابت string m_name;  // Referenced cross name

عوامی:
    AstCoverCrossRef(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_START
        واپس m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : عوامی Ast_گرہ {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ثابت bool m_isOr;  // Union (||), rather than intersection (&&)

عوامی:
    AstCoverCrossSelect(File_سطر* fl, Ast_گرہ* lhsp, Ast_گرہ* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool isOr() ثابت { واپس m_isOr; }
    string verilogKwd() ثابت override { واپس isOr() ? "||" : "&&"; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_START
        واپس m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : عوامی Ast_گرہ {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
عوامی:
    AstCoverCrossWith(File_سطر* fl, Ast_گرہ* selectp, Ast_گرہ_اظہاریہ* filterp, Ast_گرہ_اظہاریہ* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : عوامی Ast_گرہ {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ثابت VCoverOptionType m_optType;  // Option being assigned
    ثابت bool m_typeOption;  // type_option (vs option)

عوامی:
    AstCoverOption(File_سطر* fl, bool typeOption, VCoverOptionType optType, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VCoverOptionType optType() ثابت { واپس m_optType; }
    bool typeOption() ثابت { واپس m_typeOption; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverOption* ثابت asamep = VN_DBG_AS(samep, CoverOption);
        واپس m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : عوامی Ast_گرہ {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
عوامی:
    AstCoverToggle(File_سطر* fl, AstCoverInc* incp, Ast_گرہ_اظہاریہ* origp, Ast_گرہ_اظہاریہ* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    صحیح instr_گنتی() ثابت override { واپس 3 + INSTR_گنتی_BRANCH + INSTR_گنتی_LD; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس true; }
    bool isOutputter() override {
        واپس false;  // Though the AstCoverInc under this is an outputter
    }
};
class Ast_Cover_Trans_شے final : عوامی Ast_گرہ {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ثابت VTransRepType m_repType;

عوامی:
    Ast_Cover_Trans_شے(File_سطر* fl, Ast_گرہ* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_Cover_Trans_شے(fl)
        , m_repType{repType} {
        جوڑو_Valuesp(valuesp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_شے;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class Ast_Cover_Trans_سیٹ final : عوامی Ast_گرہ {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
عوامی:
    Ast_Cover_Trans_سیٹ(File_سطر* fl, Ast_Cover_Trans_شے* itemsp)
        : ASTGEN_SUPER_Cover_Trans_سیٹ(fl) {
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_سیٹ;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class AstCoverWith final : عوامی Ast_گرہ {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
عوامی:
    AstCoverWith(File_سطر* fl, Ast_گرہ* subp, Ast_گرہ_اظہاریہ* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        جوڑو_Subp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : عوامی Ast_گرہ {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

عوامی:
    AstCovergroup(File_سطر* fl, ثابت string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_گرہ* membersp, Ast_Sen_درخت* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        جوڑو_Argsp(argsp);
        جوڑو_Sample_Argsp(sampleArgsp);
        جوڑو_Membersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& name) override { m_name = name; }
    bool maybe_Pointed_مقصد() ثابت override { واپس true; }
};
class AstCoverpointRef final : عوامی Ast_گرہ {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ثابت string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

عوامی:
    AstCoverpointRef(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
};
class AstDefParam final : عوامی Ast_گرہ {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
عوامی:
    AstDefParam(File_سطر* fl, Ast_گرہ_اظہاریہ* pathp, ثابت string& name, Ast_گرہ_اظہاریہ* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس true; }
};
class AstDefaultClocking final : عوامی Ast_گرہ {
    std::string m_name;  // Clocking block name

عوامی:
    AstDefaultClocking(File_سطر* fl, ثابت std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ثابت override VL_MT_STABLE { واپس m_name; }
};
class AstDefaultDisable final : عوامی Ast_گرہ {
    // @astgen op1 := condp : AstNodeExpr

عوامی:
    AstDefaultDisable(File_سطر* fl, Ast_گرہ_اظہاریہ* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : عوامی Ast_گرہ {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
عوامی:
    AstDpiExport(File_سطر* fl, ثابت string& vname, ثابت string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت string& name) override { m_name = name; }
    string cname() ثابت { واپس m_cname; }
    خالی cname(ثابت string& cname) { m_cname = cname; }
};
class AstElabDisplay final : عوامی Ast_گرہ {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

عوامی:
    اندرونی AstElabDisplay(File_سطر* fl, VDisplayType dispType, Ast_گرہ_اظہاریہ* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        واپس nullptr;
    }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string verilogKwd() ثابت override { واپس "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ثابت override { واپس false; }
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { واپس true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { واپس true; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    صحیح instr_گنتی() ثابت override { واپس INSTR_گنتی_PLI; }
    VDisplayType displayType() ثابت { واپس m_displayType; }
    خالی displayType(VDisplayType type) { m_displayType = type; }
};
class Ast_خالی_خانے final : عوامی Ast_گرہ {
    // Represents something missing, e.g. a missing argument in FOREACH
عوامی:
    explicit Ast_خالی_خانے(File_سطر* fl)
        : ASTGEN_SUPER_خالی_خانے(fl) {}
    ASTGEN_MEMBERS_Ast_خالی_خانے;
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class Ast_Gen_Case_شے final : عوامی Ast_گرہ {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
عوامی:
    Ast_Gen_Case_شے(File_سطر* fl, Ast_گرہ_اظہاریہ* condsp, Ast_گرہ* itemsp)
        : ASTGEN_SUPER_Gen_Case_شے(fl) {
        جوڑو_Condsp(condsp);
        جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_Case_شے;
    bool isDefault() ثابت { واپس condsp() == nullptr; }
};
class AstImplicit final : عوامی Ast_گرہ {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
عوامی:
    AstImplicit(File_سطر* fl, Ast_گرہ* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        جوڑو_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : عوامی Ast_گرہ {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_والد_Name;  // Hierarchical path of the declaring module, before inlining
    string m_بنیاد_Name;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
عوامی:
    AstIntfRef(File_سطر* fl, ثابت string& name, ثابت string& والد_Name, ثابت string& بنیاد_Name,
               ثابت string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_والد_Name{والد_Name}
        , m_بنیاد_Name{بنیاد_Name}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    string والد_Name() ثابت { واپس m_والد_Name; }
    string بنیاد_Name() ثابت { واپس m_بنیاد_Name; }
    string modportName() ثابت { واپس m_modportName; }
};
class AstLibrary final : عوامی Ast_گرہ {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

عوامی:
    AstLibrary(File_سطر* fl, ثابت std::string& name, Ast_گرہ_اظہاریہ* filesp, Ast_گرہ_اظہاریہ* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        جوڑو_Filesp(filesp);
        جوڑو_Incdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ثابت override VL_MT_STABLE { واپس m_name; }
};
class AstModport final : عوامی Ast_گرہ {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
عوامی:
    AstModport(File_سطر* fl, ثابت string& name, Ast_گرہ* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        جوڑو_Varsp(varsp);
    }
    string verilogKwd() ثابت override { واپس "modport"; }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : عوامی Ast_گرہ {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
عوامی:
    AstModportClockingRef(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    خالی dump(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    AstClocking* clockingp() ثابت VL_MT_STABLE {
        واپس m_clockingp;
    }  // [After Link] Pointer to clocking block
    خالی clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : عوامی Ast_گرہ {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
عوامی:
    AstModportFTaskRef(File_سطر* fl, ثابت string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool isImport() ثابت { واپس !m_export; }
    bool isExport() ثابت { واپس m_export; }
    Ast_گرہ_FTask* ftaskp() ثابت { واپس m_ftaskp; }  // [After Link] Pointer to variable
    خالی ftaskp(Ast_گرہ_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : عوامی Ast_گرہ {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ثابت VDirection m_direction;  // Direction of the variable (in/out)
عوامی:
    AstModportVarRef(File_سطر* fl, ثابت string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_سطر* fl, ثابت string& name, Ast_گرہ_اظہاریہ* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    VDirection direction() ثابت { واپس m_direction; }
    AstVar* varp() ثابت VL_MT_STABLE { واپس m_varp; }  // [After Link] Pointer to variable
    خالی varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : عوامی Ast_گرہ {
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
    std::string m_resolved_چوٹی_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::سیٹ<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ثابت AstConst*, string> m_const_Orig_Param_نام;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ثابت Ast_گرہ*, ثابت Ast_گرہ_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::ENUM_انتہا> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::ENUM_انتہا> m_dumpTriggersFuncps{};

عوامی:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ثابت حرف* broken() ثابت override;
    خالی ڈالو_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.داخل_کرو(varp); }
    ثابت std::سیٹ<AstVar*>& deferredParamVarps() ثابت { واپس m_deferredParamVarps; }
    خالی clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    خالی deleteContents();
    خالی cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ثابت override VL_MT_STABLE { واپس "$root"; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    Ast_گرہ_Module* چوٹی_Modulep() ثابت VL_MT_STABLE {  // Top module in hierarchy
        واپس modulesp();  // First one in the list, for now
    }
    Ast_Type_جدول* typeTablep() { واپس m_typeTablep; }
    string astConstOrigParamName(ثابت AstConst* nodep) ثابت;
    خالی astConstOrigParamName(ثابت AstConst* nodep, ثابت string& name);
    خالی astConstOrigParamNameErase(ثابت AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ثابت Ast_گرہ_Module* containingModule(ثابت Ast_گرہ* nodep);
    // Forget remembered modules, as the tree has moved.
    خالی clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ثابت { واپس m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ثابت { واپس m_dollarUnitPkgp; }
    خالی dollarUnitPkgp(AstPackage* ثابت packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ثابت { واپس m_dpiExportTriggerp; }
    خالی dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ثابت { واپس m_delaySchedulerp; }
    خالی delaySchedulerp(AstVar* ثابت varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ثابت { واپس m_nbaEventp; }
    خالی nbaEventp(AstVarScope* ثابت varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ثابت { واپس m_nbaEventTriggerp; }
    خالی nbaEventTriggerp(AstVarScope* ثابت varScopep) { m_nbaEventTriggerp = varScopep; }
    خالی stdPackagep(AstPackage* ثابت packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ثابت { واپس m_stdPackagep; }
    خالی stdPackageProcessp(AstClass* ثابت classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ثابت { واپس m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_سطر*) ثابت;
    Ast_چوٹی_Scope* چوٹی_Scopep() ثابت { واپس m_چوٹی_Scopep; }
    خالی create_چوٹی_Scope(AstScope* scopep);
    VTimescale timeunit() ثابت { واپس m_timeunit; }
    خالی timeunit(ثابت VTimescale& قدر) { m_timeunit = قدر; }
    VTimescale timeprecision() ثابت { واپس m_timeprecision; }
    خالی وقت_Init() {
        m_timeunit = v3Global.opt.وقت_Default_Unit();
        m_timeprecision = v3Global.opt.وقت_Default_Prec();
    }
    خالی timeprecision_ملاؤ(File_سطر*, ثابت VTimescale& قدر);
    خالی timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ثابت { واپس m_timescaleSpecified; }
    ثابت std::string& resolved_چوٹی_Module_Name() ثابت { واپس m_resolved_چوٹی_Module_Name; }
    خالی resolved_چوٹی_Module_Name(ثابت std::string& قدر) { m_resolved_چوٹی_Module_Name = قدر; }
    uint32_t nTraceCodes() ثابت { واپس m_nTraceCodes; }
    خالی nTraceCodes(uint32_t قدر) { m_nTraceCodes = قدر; }
    AstVarScope* stl_پہلا_Iterationp();
    خالی clear_Stl_پہلا_Iterationp() { m_stl_پہلا_Iterationp = nullptr; }
    ثابت std::string trace_Lib_چوٹی_Name() ثابت {
        ثابت std::string& name = resolved_چوٹی_Module_Name();
        واپس prettyName(name.خالی_خانے() ? v3Global.rootp()->چوٹی_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    خالی جوڑو_Eval_Stats(ثابت std::string& phase);
};
class AstPackageExport final : عوامی Ast_گرہ {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

عوامی:
    AstPackageExport(File_سطر* fl, ثابت string& pkgName, ثابت string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    string pkgName() ثابت VL_MT_STABLE { واپس m_pkgName; }
    string prettyPkgNameQ() ثابت { واپس "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ثابت { واپس m_packagep; }
    خالی packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class Ast_Package_Export_ستارہ_ستارہ final : عوامی Ast_گرہ {
    // A package export *::* declaration
عوامی:
    // cppcheck-suppress noExplicitConstructor
    Ast_Package_Export_ستارہ_ستارہ(File_سطر* fl)
        : ASTGEN_SUPER_Package_Export_ستارہ_ستارہ(fl) {}
    ASTGEN_MEMBERS_Ast_Package_Export_ستارہ_ستارہ;
};
class AstPackageImport final : عوامی Ast_گرہ {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

عوامی:
    AstPackageImport(File_سطر* fl, AstPackage* packagep, ثابت string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_سطر* fl, ثابت string& pkgName, ثابت string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    string pkgName() ثابت VL_MT_STABLE { واپس m_pkgName; }
    string prettyPkgNameQ() ثابت { واپس "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ثابت { واپس m_packagep; }
    خالی packagep(AstPackage* nodep) { m_packagep = nodep; }

نجی:
    خالی pkgNameFrom();
};
class AstPin final : عوامی Ast_گرہ {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    صحیح m_pin_عدد;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_param_راستہ;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
عوامی:
    AstPin(File_سطر* fl, صحیح pin_عدد, ثابت string& name, Ast_گرہ* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_عدد{pin_عدد}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    خالی cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Pin name, ""=go by number
    خالی name(ثابت string& name) override { m_name = name; }
    string prettyOperatorName() ثابت override;
    bool dot_ستارہ() ثابت { واپس name() == ".*"; }  // Fake name for .* connections until linked
    صحیح pin_عدد() ثابت { واپس m_pin_عدد; }
    AstVar* modVarp() ثابت { واپس m_modVarp; }  // [After Link] Pointer to variable
    خالی modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ثابت { واپس m_modPTypep; }
    خالی modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ثابت { واپس m_param; }
    خالی param(bool flag) { m_param = flag; }
    ثابت string& param_راستہ() ثابت { واپس m_param_راستہ; }
    خالی param_راستہ(ثابت string& راستہ) { m_param_راستہ = راستہ; }
    bool svDotName() ثابت { واپس m_svDotName; }
    خالی svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ثابت { واپس m_svImplicit; }
    خالی svImplicit(bool flag) { m_svImplicit = flag; }
};
class Ast_پورٹ final : عوامی Ast_گرہ {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ثابت صحیح m_pin_عدد;  // Pin number
    ثابت string m_name;  // Name of pin
عوامی:
    Ast_پورٹ(File_سطر* fl, صحیح pinnum, ثابت string& name)
        : ASTGEN_SUPER_پورٹ(fl)
        , m_pin_عدد{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_پورٹ;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Port name
    صحیح pin_عدد() ثابت { واپس m_pin_عدد; }  // * = Pin number, for order based instantiation
};
class AstPragma final : عوامی Ast_گرہ {
    ثابت VPragmaType m_pragType;  // Type of pragma
    ثابت VTimescale m_timescale;  // For TIMEUNIT_SET
عوامی:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_سطر* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_سیٹ, fl, "Should use other constructor");
    }
    AstPragma(File_سطر* fl, ثابت VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_سیٹ}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VPragmaType pragType() ثابت { واپس m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ثابت override { واپس false; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ثابت { واپس m_timescale; }
};
class AstPropSpec final : عوامی Ast_گرہ {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

عوامی:
    AstPropSpec(File_سطر* fl, Ast_Sen_شے* sensesp, Ast_گرہ_اظہاریہ* disablep, Ast_گرہ* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ {
        واپس true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ثابت { واپس m_propStrength; }
};
class AstPull final : عوامی Ast_گرہ {
    // @astgen op1 := lhsp : AstNodeExpr

    ثابت bool m_direction;

عوامی:
    AstPull(File_سطر* fl, Ast_گرہ_اظہاریہ* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ثابت { واپس m_direction; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class AstScope final : عوامی Ast_گرہ {
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
عوامی:
    AstScope(File_سطر* fl, Ast_گرہ_Module* modp, ثابت string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!m_modp);
        واپس nullptr;
    }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Scope name
    خالی name(ثابت string& name) override { m_name = name; }
    string pretty_Name_پیغام() ثابت override;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    string nameDotless() ثابت;
    Ast_گرہ_Module* modp() ثابت { واپس m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    Ast_فعال* comboActivep(bool create);
    //
    AstScope* aboveScopep() ثابت VL_MT_محفوظ { واپس m_aboveScopep; }
    خالی aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ثابت { واپس m_aboveCellp; }
    خالی aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_چوٹی() ثابت VL_MT_محفوظ { واپس aboveScopep() == nullptr; }  // At top of hierarchy
};
class Ast_Sen_شے final : عوامی Ast_گرہ {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_کنارہ_Type;  // Edge type
عوامی:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    Ast_Sen_شے(File_سطر* fl, VEdgeType کنارہ_Type, Ast_گرہ_اظہاریہ* senp, Ast_گرہ_اظہاریہ* condp = nullptr)
        : ASTGEN_SUPER_Sen_شے(fl)
        , m_کنارہ_Type{کنارہ_Type} {
        this->sensp(senp);
        this->condp(condp);
    }
    Ast_Sen_شے(File_سطر* fl, Combo)
        : ASTGEN_SUPER_Sen_شے(fl)
        , m_کنارہ_Type{VEdgeType::ET_COMBO} {}
    Ast_Sen_شے(File_سطر* fl, Static)
        : ASTGEN_SUPER_Sen_شے(fl)
        , m_کنارہ_Type{VEdgeType::ET_STATIC} {}
    Ast_Sen_شے(File_سطر* fl, Initial)
        : ASTGEN_SUPER_Sen_شے(fl)
        , m_کنارہ_Type{VEdgeType::ET_INITIAL} {}
    Ast_Sen_شے(File_سطر* fl, InitialNBA)
        : ASTGEN_SUPER_Sen_شے(fl)
        , m_کنارہ_Type{VEdgeType::ET_INITIAL_NBA} {}
    Ast_Sen_شے(File_سطر* fl, Final)
        : ASTGEN_SUPER_Sen_شے(fl)
        , m_کنارہ_Type{VEdgeType::ET_FINAL} {}
    Ast_Sen_شے(File_سطر* fl, Never)
        : ASTGEN_SUPER_Sen_شے(fl)
        , m_کنارہ_Type{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_Ast_Sen_شے;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس کنارہ_Type() == VN_DBG_AS(samep, Sen_شے)->کنارہ_Type();
    }
    VEdgeType کنارہ_Type() ثابت { واپس m_کنارہ_Type; }
    خالی کنارہ_Type(VEdgeType type) {
        m_کنارہ_Type = type;
        edit_گنتی_Inc();
    }
    Ast_گرہ_Var_Ref* varrefp() ثابت { واپس VN_CAST(sensp(), گرہ_Var_Ref); }
    //
    bool isClocked() ثابت { واپس کنارہ_Type().clockedStmt(); }
    bool is_Combo_Or_ستارہ() ثابت {
        واپس کنارہ_Type() == VEdgeType::ET_COMBO || کنارہ_Type() == VEdgeType::ET_COMBO_ستارہ;
    }
    bool is_Combo_ستارہ() ثابت { واپس کنارہ_Type() == VEdgeType::ET_COMBO_ستارہ; }
    bool isHybrid() ثابت { واپس کنارہ_Type() == VEdgeType::ET_HYBRID; }
    bool isStatic() ثابت { واپس کنارہ_Type() == VEdgeType::ET_STATIC; }
    bool isInitial() ثابت { واپس کنارہ_Type() == VEdgeType::ET_INITIAL; }
    bool isFinal() ثابت { واپس کنارہ_Type() == VEdgeType::ET_FINAL; }
    bool isNever() ثابت { واپس کنارہ_Type() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_درخت final : عوامی Ast_گرہ {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
عوامی:
    Ast_Sen_درخت(File_سطر* fl, Ast_Sen_شے* sensesp)
        : ASTGEN_SUPER_Sen_درخت(fl) {
        جوڑو_Sensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_درخت;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    // METHODS
    bool hasClocked() ثابت;  // Includes a clocked statement
    bool has_کنارہ() ثابت;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ثابت;  // Includes a STATIC SenItem
    bool hasInitial() ثابت;  // Includes a INITIAL SenItem
    bool hasFinal() ثابت;  // Includes a FINAL SenItem
    bool hasCombo() ثابت;  // Includes a COMBO SenItem
    bool hasHybrid() ثابت;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : عوامی Ast_گرہ {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

عوامی:
    AstStrengthSpec(File_سطر* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { واپس m_s0; }
    VStrength strength1() { واپس m_s1; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class AstSystemCSection final : عوامی Ast_گرہ {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ثابت VSystemCSectionType m_sectionType;  // The section type
    std::string m_متن;  // The text content

عوامی:
    AstSystemCSection(File_سطر* fl, VSystemCSectionType sectionType, ثابت std::string& متن)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_متن{متن} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ثابت { واپس m_sectionType; }
    ثابت std::string& متن() ثابت { واپس m_متن; }
    خالی متن(ثابت std::string& قدر) { m_متن = قدر; }
    خالی dump(std::ostream&) ثابت override;
    خالی dumpJson(std::ostream&) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ*) ثابت override { واپس false; }
    bool is_خالص() override { واپس false; }
    bool isOutputter() override { واپس true; }
};
class Ast_متن final : عوامی Ast_گرہ {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ثابت std::string m_متن;  // The text to emit
عوامی:
    Ast_متن(File_سطر* fl, ثابت std::string& متن)
        : ASTGEN_SUPER_متن(fl)
        , m_متن{متن} {}
    ASTGEN_MEMBERS_Ast_متن;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس متن() == VN_DBG_AS(samep, متن_2)->متن();
    }
    ثابت std::string& متن() ثابت VL_MT_محفوظ { واپس m_متن; }
};
class Ast_متن_بلاک final : عوامی Ast_گرہ {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_سابقہ;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
عوامی:
    explicit Ast_متن_بلاک(File_سطر* fl,  //
                          ثابت std::string& سابقہ = "",  //
                          ثابت std::string& separator = "",  //
                          ثابت std::string& suffix = "")
        : ASTGEN_SUPER_متن_بلاک(fl)
        , m_سابقہ{سابقہ}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_Ast_متن_بلاک;
    ثابت std::string& سابقہ() ثابت { واپس m_سابقہ; }
    ثابت std::string& separator() ثابت { واپس m_separator; }
    ثابت std::string& suffix() ثابت { واپس m_suffix; }
    // Add some text, or a node to this block
    خالی جوڑو(ثابت string& متن) { جوڑو_Nodesp(new Ast_متن{fileline(), متن}); }
    خالی جوڑو(Ast_گرہ* nodep) { جوڑو_Nodesp(nodep); }
};
class Ast_چوٹی_Scope final : عوامی Ast_گرہ {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    دوست class AstNetlist;  // Only the AstNetlist can create one
    Ast_چوٹی_Scope(File_سطر* fl, AstScope* ascopep)
        : ASTGEN_SUPER_چوٹی_Scope(fl) {
        this->scopep(ascopep);
    }

عوامی:
    ASTGEN_MEMBERS_Ast_چوٹی_Scope;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_عارضی(ثابت string& name, غیرعلامتی چوڑائی);
    AstVarScope* create_عارضی(ثابت string& name, Ast_گرہ_DType* dtypep);
};
class Ast_Type_جدول final : عوامی Ast_گرہ {
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
    using DetailedMap = std::map<VBasic_Type_کلید, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

عوامی:
    explicit Ast_Type_جدول(File_سطر* fl);
    ASTGEN_MEMBERS_Ast_Type_جدول;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    خالی cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* ڈھونڈو_Basic_DType(File_سطر* fl, VBasicDTypeKwd kwd);
    AstBasicDType* ڈھونڈو_Logic_ثنائی_ہندسہ_DType(File_سطر* fl, VBasicDTypeKwd kwd, صحیح چوڑائی, صحیح چوڑائی_Min,
                                     VSigning numeric);
    AstBasicDType* ڈھونڈو_Logic_ثنائی_ہندسہ_DType(File_سطر* fl, VBasicDTypeKwd kwd, ثابت VNumRange& range,
                                     صحیح چوڑائی_Min, VSigning numeric);
    AstBasicDType* ڈھونڈو_Create_Same_DType(AstBasicDType& گرہ);
    AstBasicDType* ڈھونڈو_داخل_کرو_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* ڈھونڈو_Constraint_Ref_DType(File_سطر* fl);
    Ast_خالی_خانے_قطار_DType* ڈھونڈو_خالی_خانے_قطار_DType(File_سطر* fl);
    Ast_قطار_DType* ڈھونڈو_قطار_اشاریہ_DType(File_سطر* fl);
    Ast_دھارا_DType* ڈھونڈو_دھارا_DType(File_سطر* fl);
    AstVoidDType* ڈھونڈو_Void_DType(File_سطر* fl);
    خالی clearCache();
    خالی repairCache();
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstTypedef final : عوامی Ast_گرہ {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_لیبل;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_عدد;  // Declaration token number
    bool m_attr_عوامی = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

عوامی:
    AstTypedef(File_سطر* fl, ثابت string& name, Ast_گرہ* attrsp, VFlag_بچہ_DType,
               Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_عدد{fl->token_عدد()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        بچہ_DTypep(dtp);  // Only for parser
        جوڑو_Attrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_سطر* fl, ثابت string& name, Ast_گرہ_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_عدد{fl->token_عدد()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ثابت;
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    مجازی Ast_گرہ_DType* subDTypep() ثابت VL_MT_STABLE {
        واپس dtypep() ? dtypep() : بچہ_DTypep();
    }
    // METHODS
    uint32_t decl_Token_عدد() ثابت override { واپس m_decl_Token_عدد; }
    خالی decl_Token_عدد_سیٹ_Min(uint32_t token_عدد) override {
        m_decl_Token_عدد = std::min(m_decl_Token_عدد, token_عدد);
    }
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    خالی name(ثابت string& flag) override { m_name = flag; }
    bool attr_عوامی() ثابت { واپس m_attr_عوامی; }
    خالی attr_عوامی(bool flag) { m_attr_عوامی = flag; }
    bool isHideLocal() ثابت { واپس m_isHideLocal; }
    خالی isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ثابت { واپس m_isHideProtected; }
    خالی isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ثابت { واپس m_isUnderClass; }
    خالی isUnderClass(bool flag) { m_isUnderClass = flag; }
    خالی لیبل(ثابت string& متن) override { m_لیبل = متن; }
    string لیبل() ثابت override { واپس m_لیبل; }
};
class AstTypedefFwd final : عوامی Ast_گرہ {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ثابت VFwdType m_fwdType;  // Forward type for lint check

عوامی:
    AstTypedefFwd(File_سطر* fl, ثابت string& name, ثابت VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    VFwdType fwdType() ثابت { واپس m_fwdType; }
};
class Ast_Udp_جدول final : عوامی Ast_گرہ {
    // @astgen op1 := linesp : List[AstUdpTableLine]
عوامی:
    Ast_Udp_جدول(File_سطر* fl, Ast_Udp_جدول_سطر* linesp)
        : ASTGEN_SUPER_Udp_جدول(fl) {
        this->جوڑو_Linesp(linesp);
        اگر (!v3Global.has_جدول()) v3Global.سیٹ_Has_جدول();
    }
    ASTGEN_MEMBERS_Ast_Udp_جدول;
};
class Ast_Udp_جدول_سطر final : عوامی Ast_گرہ {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
نجی:
    ثابت bool m_udpIsCombo;  // Combinational or sequential UDP

عوامی:
    class UdpCombo {};
    Ast_Udp_جدول_سطر(UdpCombo, File_سطر* fl, Ast_Udp_جدول_سطر_Val* iFieldsp,
                    Ast_Udp_جدول_سطر_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_جدول_سطر(fl)
        , m_udpIsCombo{true} {
        جوڑو_IFieldsp(iFieldsp);
        جوڑو_OFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_جدول_سطر(UdpSequential, File_سطر* fl, Ast_Udp_جدول_سطر_Val* iFieldsp,
                    Ast_Udp_جدول_سطر_Val* oFieldsp1, Ast_Udp_جدول_سطر_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_جدول_سطر(fl)
        , m_udpIsCombo{false} {
        جوڑو_IFieldsp(iFieldsp);
        جوڑو_OFieldsp(oFieldsp1);
        جوڑو_OFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_جدول_سطر;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح udpIsCombo() ثابت { واپس m_udpIsCombo; }
};
class Ast_Udp_جدول_سطر_Val final : عوامی Ast_گرہ {
    string m_متن;  // Value character

عوامی:
    Ast_Udp_جدول_سطر_Val(File_سطر* fl, ثابت string& متن)
        : ASTGEN_SUPER_Udp_جدول_سطر_Val(fl)
        , m_متن{متن} {}
    ASTGEN_MEMBERS_Ast_Udp_جدول_سطر_Val;
    string name() ثابت override VL_MT_STABLE { واپس m_متن; }
    خالی name(std::string ثابت& متن) override VL_MT_STABLE { m_متن = متن; }
    string متن() ثابت VL_MT_محفوظ { واپس m_متن; }
};
class AstVar final : عوامی Ast_گرہ {
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
    string m_لیبل;  // Holds the string of the verilator tag -- used in JSON output.
    VVarType m_varType;  // Type of variable
    VDirection m_direction;  // Direction input/output etc
    VDirection m_declDirection;  // Declared direction input/output etc
    VLifetime m_lifetime;  // Lifetime
    VRandAttr m_rand;  // Randomizability of this variable (rand, randc, etc)
    صحیح m_pin_عدد = 0;  // For JSON, if non-zero the connection pin number
    صحیح m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
    bool m_libInoutEnable : 1;  // Port is the lowered library inout's enable
    bool m_ansi : 1;  // Params or pins declared in the module header, rather than the body
    bool m_declTyped : 1;  // Declared as type (for dedup check)
    bool m_tristate : 1;  // Inout or triwire or trireg
    bool m_primaryIO : 1;  // In/out to top level (or directly assigned from same)
    bool m_primaryClock : 1;  // In/out to top level and is, or combinationally drives a SenItem
    bool m_sc : 1;  // SystemC variable
    bool m_scSensitive : 1;  // SystemC sensitive() needed
    bool m_sig_عوامی : 1;  // User C code accesses this signal or is top signal
    bool m_sig_Mod_عوامی : 1;  // User C code accesses this signal and module
    bool m_sig_User_Rd_عوامی : 1;  // User C code accesses this signal, read only
    bool m_sigUserRWPublic : 1;  // User C code accesses this signal, read-write
    bool m_مستعمل_Param : 1;  // Parameter is referenced (on link; later signals not setup)
    bool m_مستعمل_Loop_Idx : 1;  // Variable subject of for unrolling
    bool m_funcLocal : 1;  // Local variable for a function
    bool m_funcLocalSticky : 1;  // As m_funcLocal but remains set if var is moved to a static
    bool m_funcReturn : 1;  // Return variable for a function
    bool m_attrScBv : 1;  // User force bit vector attribute
    bool m_attrScBigUint : 1;  // User force sc_biguint attribute
    bool m_attrSFormat : 1;  // User sformat attribute
    bool m_attr_بانٹو_Var : 1;  // declared with split_var metacomment
    bool m_attrFsmState : 1;  // declared with fsm_state metacomment
    bool m_attrFsmArcInclCond : 1;  // declared with fsm_arc_include_cond metacomment
    VFsmExpandType::en m_attrFsmStateExpand : 2;  // FSM expansion type for FSM coverage
    bool m_const_Pool_اندراج : 1;  // Constant pool variable
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
    bool m_is_Iface_والد : 1;  // dtype is reference to interface present in this module
    bool m_is_Iface_Array_بانٹو : 1;  // Interface array that was split into element variables by
                                   // V3Param, until V3Width removes it
    bool m_is_اندرونی : 1;  // Internal state, don't add to method pinter
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
    bool m_is_Forced_By_رمز : 1;  // May be forced/released from AstAssignForce/AstRelease
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
    bool m_process_قطار : 1;  // Process queue variable
    bool m_mtask_Cache_سطر_Align : 1;  // Start MTask affinity group on a cache line
    خالی init() {
        m_libInoutEnable = false;
        m_ansi = false;
        m_declTyped = false;
        m_tristate = false;
        m_primaryIO = false;
        m_primaryClock = false;
        m_sc = false;
        m_scSensitive = false;
        m_مستعمل_Param = false;
        m_مستعمل_Loop_Idx = false;
        m_sig_عوامی = false;
        m_sig_Mod_عوامی = false;
        m_sig_User_Rd_عوامی = false;
        m_sigUserRWPublic = false;
        m_funcLocal = false;
        m_funcLocalSticky = false;
        m_funcReturn = false;
        m_attrScBv = false;
        m_attrScBigUint = false;
        m_attrSFormat = false;
        m_attr_بانٹو_Var = false;
        m_attrFsmState = false;
        m_attrFsmArcInclCond = false;
        m_attrFsmStateExpand = VFsmExpandType::DEFAULT;
        m_const_Pool_اندراج = false;
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
        m_is_Iface_والد = false;
        m_is_Iface_Array_بانٹو = false;
        m_is_اندرونی = false;
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
        m_is_Forced_By_رمز = false;
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
        m_process_قطار = false;
        m_mtask_Cache_سطر_Align = false;
    }

عوامی:
    AstVar(File_سطر* fl, VVarType type, ثابت string& name, VFlag_بچہ_DType, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        بچہ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_سطر* fl, VVarType type, ثابت string& name, Ast_گرہ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_سطر* fl, VVarType type, ثابت string& name, VFlagLogicPacked, صحیح wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_سیٹ_Logic_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_سطر* fl, VVarType type, ثابت string& name, VFlag_ثنائی_ہندسہ_Packed, صحیح wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_سیٹ_ثنائی_ہندسہ_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_سطر* fl, VVarType type, ثابت string& name, ثابت AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        اگر (examplep->بچہ_DTypep()) بچہ_DTypep(examplep->بچہ_DTypep()->clone_درخت(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    اندرونی bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { واپس m_name; }  // * = Var name
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    string origName() ثابت override { واپس m_origName; }  // * = Original name
    خالی origName(ثابت string& name) { m_origName = name; }
    VVarType varType() ثابت VL_MT_محفوظ { واپس m_varType; }  // * = Type of variable
    خالی direction(ثابت VDirection& flag) {
        m_direction = flag;
        اگر (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ثابت VL_MT_محفوظ { واپس m_direction; }
    bool isIO() ثابت VL_MT_محفوظ { واپس m_direction != VDirection::NONE; }
    bool isVLIO() ثابت {
        ثابت AstBasicDType* ثابت bdtypep = basicp();
        واپس isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    خالی declDirection(ثابت VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ثابت { واپس m_declDirection; }
    صحیح libInoutId() ثابت { واپس m_libInoutId; }
    خالی libInoutId(صحیح id) { m_libInoutId = id; }
    bool libInoutEnable() ثابت { واپس m_libInoutEnable; }
    خالی libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    خالی varType(VVarType type) { m_varType = type; }
    خالی var_Type2_برآمد() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    خالی var_Type2_اندر() {
        m_tristate = false;
        m_direction = VDirection::داخلہ;
    }
    string scType() ثابت;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) ثابت;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) ثابت;
    string dpiTmpVarType(ثابت string& varName) ثابت;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) ثابت;
    string vlEnumType() ثابت;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) ثابت;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(ثابت string& propName) ثابت;  // Return VerilatorVarProps declaration
    خالی combineType(VVarType type);
    Ast_گرہ_DType* get_بچہ_DTypep() ثابت override { واپس بچہ_DTypep(); }
    Ast_گرہ_DType* dtypeSkipRefp() ثابت VL_MT_STABLE { واپس subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ثابت VL_MT_STABLE { واپس subDTypep()->basicp(); }
    مجازی Ast_گرہ_DType* subDTypep() ثابت VL_MT_STABLE {
        واپس dtypep() ? dtypep() : بچہ_DTypep();
    }
    خالی ansi(bool flag) { m_ansi = flag; }
    خالی declTyped(bool flag) { m_declTyped = flag; }
    خالی sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    خالی attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    خالی attrScBv(bool flag) { m_attrScBv = flag; }
    خالی attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    خالی attrSFormat(bool flag) { m_attrSFormat = flag; }
    خالی attr_بانٹو_Var(bool flag) { m_attr_بانٹو_Var = flag; }
    خالی attrFsmState(bool flag) { m_attrFsmState = flag; }
    خالی attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    خالی attrFsmStateExpand(ثابت VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool const_Pool_اندراج() ثابت { واپس m_const_Pool_اندراج; }
    خالی سیٹ_Const_Pool_اندراج() { m_const_Pool_اندراج = true; }
    bool covergroupRefMember() ثابت { واپس m_covergroupRefMember; }
    خالی covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ثابت { واپس m_embeddedCovergroup; }
    خالی embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    خالی rand(ثابت VRandAttr flag) { m_rand = flag; }
    خالی مستعمل_Param(bool flag) { m_مستعمل_Param = flag; }
    خالی مستعمل_Loop_Idx(bool flag) { m_مستعمل_Loop_Idx = flag; }
    خالی sig_عوامی(bool flag) { m_sig_عوامی = flag; }
    خالی sig_Mod_عوامی(bool flag) { m_sig_Mod_عوامی = flag; }
    خالی sig_User_Rd_عوامی(bool flag) {
        m_sig_User_Rd_عوامی = flag;
        اگر (flag) sig_عوامی(true);
    }
    خالی sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        اگر (flag) sig_User_Rd_عوامی(true);
    }
    خالی sc(bool flag) { m_sc = flag; }
    خالی scSensitive(bool flag) { m_scSensitive = flag; }
    خالی primaryIO(bool flag) { m_primaryIO = flag; }
    خالی isConst(bool flag) { m_isConst = flag; }
    خالی isContinuously(bool flag) { m_isContinuously = flag; }
    خالی is_Iface_والد(bool flag) { m_is_Iface_والد = flag; }
    خالی is_Iface_Array_بانٹو(bool flag) { m_is_Iface_Array_بانٹو = flag; }
    خالی is_اندرونی(bool flag) { m_is_اندرونی = flag; }
    خالی isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    خالی funcLocal(bool flag) {
        m_funcLocal = flag;
        اگر (flag) m_funcLocalSticky = true;
    }
    خالی funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ثابت { واپس m_gotNansiType; }
    خالی gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ثابت { واپس m_hasStrengthAssignment; }
    خالی hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ثابت { واپس m_hasUserInit; }
    خالی hasUserInit(bool flag) { m_hasUserInit = flag; }
    خالی icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ثابت { واپس m_icoMaybeWritten; }
    خالی isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ثابت { واپس m_isClockingDriven; }
    bool isDpiOpenArray() ثابت VL_MT_محفوظ { واپس m_isDpiOpenArray; }
    خالی isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ثابت { واپس m_isHideLocal; }
    خالی isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ثابت { واپس m_isHideProtected; }
    خالی isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ثابت { واپس m_noCReset; }
    خالی noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ثابت { واپس m_noReset; }
    خالی noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ثابت { واپس m_noSubst; }
    خالی noSubst(bool flag) { m_noSubst = flag; }
    bool process_قطار() ثابت { واپس m_process_قطار; }
    خالی process_قطار(bool flag) { m_process_قطار = flag; }
    bool sampled() ثابت { واپس m_sampled; }
    خالی sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ثابت { واپس m_substConstOnly; }
    خالی substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ثابت { واپس m_overriddenParam; }
    خالی overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ثابت { واپس m_untypedParam; }
    خالی untypedParam(bool flag) { m_untypedParam = flag; }
    خالی trace(bool flag) { m_trace = flag; }
    خالی isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ثابت { واپس m_isForceable; }
    خالی سیٹ_Forceable() { m_isForceable = true; }
    خالی سیٹ_Forced_By_رمز() { m_is_Forced_By_رمز = true; }
    bool isForced() ثابت { واپس m_isForceable || m_is_Forced_By_رمز; }
    bool isReadByDpi() ثابت { واپس m_isReadByDpi; }
    خالی سیٹ_Read_By_Dpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ثابت { واپس m_isWrittenByDpi; }
    خالی سیٹ_Written_By_Dpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ثابت { واپس m_isWrittenBySuspendable; }
    خالی سیٹ_Written_By_Suspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ثابت { واپس m_ignorePostRead; }
    خالی سیٹ_Ignore_Post_Read() { m_ignorePostRead = true; }
    bool ignorePostWrite() ثابت { واپس m_ignorePostWrite; }
    خالی سیٹ_Ignore_Post_Write() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ثابت { واپس m_ignoreSchedWrite; }
    خالی سیٹ_Ignore_Sched_Write() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ثابت { واپس m_dfgTriLowered; }
    خالی سیٹ_Dfg_Tri_Lowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ثابت { واپس m_dfgAllowMultidriveTri; }
    خالی سیٹ_Dfg_Allow_Multidrive_Tri() { m_dfgAllowMultidriveTri = true; }
    خالی globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ثابت { واپس m_globalConstrained; }
    bool isStdRandomizeArg() ثابت { واپس m_isStdRandomizeArg; }
    خالی سیٹ_Std_Randomize_Arg() { m_isStdRandomizeArg = true; }
    // METHODS
    خالی name(ثابت string& name) override { m_name = name; }
    خالی لیبل(ثابت string& متن) override { m_لیبل = متن; }
    string لیبل() ثابت override { واپس m_لیبل; }
    bool isAnsi() ثابت { واپس m_ansi; }
    bool isContinuously() ثابت { واپس m_isContinuously; }
    bool isDeclTyped() ثابت { واپس m_declTyped; }
    bool isInout() ثابت { واپس m_direction.isInout(); }
    bool isInoutOrRef() ثابت { واپس m_direction.isInoutOrRef(); }
    bool is_داخلہ() ثابت { واپس m_direction.is_داخلہ(); }
    bool isNonOutput() ثابت { واپس m_direction.isNonOutput(); }
    bool isReadOnly() ثابت VL_MT_محفوظ { واپس m_direction.isReadOnly(); }
    bool isConstRef() ثابت VL_MT_محفوظ { واپس m_direction.isConstRef(); }
    bool isRef() ثابت VL_MT_محفوظ { واپس m_direction.isRef(); }
    bool isWritable() ثابت VL_MT_محفوظ { واپس m_direction.isWritable(); }
    bool isTristate() ثابت { واپس m_tristate; }
    bool isPrimaryIO() ثابت VL_MT_محفوظ { واپس m_primaryIO; }
    bool isPrimaryInish() ثابت { واپس isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ثابت VL_MT_محفوظ { واپس m_primaryClock; }
    خالی سیٹ_Primary_Clock() { m_primaryClock = true; }
    bool isIfaceRef() ثابت { واپس varType() == VVarType::IFACEREF; }
    خالی سیٹ_Iface_Ref() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool is_Iface_والد() ثابت { واپس m_is_Iface_والد; }
    bool is_Iface_Array_بانٹو() ثابت { واپس m_is_Iface_Array_بانٹو; }
    bool is_اندرونی() ثابت { واپس m_is_اندرونی; }
    bool isIfaceParam() ثابت { واپس m_isIfaceParam; }
    bool isSignal() ثابت { واپس varType().isSignal(); }
    bool isNet() ثابت { واپس varType().isNet(); }
    bool isWor() ثابت { واپس varType().isWor(); }
    bool isWiredNet() ثابت { واپس varType().isWiredNet(); }
    bool is_عارضی() ثابت { واپس varType().is_عارضی(); }
    bool isClassMember() ثابت { واپس varType() == VVarType::MEMBER; }
    bool isVirtIface() ثابت {
        اگر (AstIfaceRefDType* ثابت dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            واپس dtp->isVirtual();
        }
        واپس false;
    }
    bool is_Statement_عارضی() ثابت { واپس varType() == VVarType::STMTTEMP; }
    bool isXTemp() ثابت { واپس varType() == VVarType::XTEMP; }
    bool isParam() ثابت { واپس varType().isParam(); }
    bool isGParam() ثابت { واپس varType() == VVarType::GPARAM; }
    bool isGenVar() ثابت { واپس varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ثابت {
        واپس isIO() || isSignal() || isClassMember() || is_عارضی() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ثابت {
        واپس !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool is_ثنائی_ہندسہ_Logic() ثابت {
        ثابت AstBasicDType* ثابت bdtypep = basicp();
        واپس bdtypep && bdtypep->is_ثنائی_ہندسہ_Logic();
    }
    bool is_مستعمل_Param() ثابت { واپس m_مستعمل_Param; }
    bool is_مستعمل_Loop_Idx() ثابت { واپس m_مستعمل_Loop_Idx; }
    bool mtask_Cache_سطر_Align() ثابت { واپس m_mtask_Cache_سطر_Align; }
    خالی mtask_Cache_سطر_Align(bool flag) { m_mtask_Cache_سطر_Align = flag; }
    bool isSc() ثابت VL_MT_محفوظ { واپس m_sc; }
    bool isScQuad() ثابت;
    bool isScBv() ثابت VL_MT_STABLE;
    bool isScUint() ثابت;
    bool isScUintBool() ثابت;
    bool isScBigUint() ثابت VL_MT_STABLE;
    bool isScSensitive() ثابت { واپس m_scSensitive; }
    bool is_Sig_عوامی() ثابت;
    bool is_Sig_Mod_عوامی() ثابت { واپس m_sig_Mod_عوامی && !isIfaceRef(); }
    bool is_Sig_User_Rd_عوامی() ثابت { واپس m_sig_User_Rd_عوامی && !isIfaceRef(); }
    bool isSigUserRWPublic() ثابت { واپس m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ثابت { واپس m_trace; }
    bool isRand() ثابت { واپس m_rand.isRand(); }
    bool isRandC() ثابت { واپس m_rand.isRandC(); }
    bool isConst() ثابت VL_MT_محفوظ { واپس m_isConst; }
    bool isLatched() ثابت { واپس m_isLatched; }
    bool isFuncLocal() ثابت { واپس m_funcLocal; }
    bool isFuncLocalSticky() ثابت { واپس m_funcLocalSticky; }
    bool isFuncReturn() ثابت { واپس m_funcReturn; }
    bool isPullup() ثابت { واپس m_isPullup; }
    bool isPulldown() ثابت { واپس m_isPulldown; }
    bool attrScBv() ثابت { واپس m_attrScBv; }
    bool attrScBigUint() ثابت { واپس m_attrScBigUint; }
    bool attrFileDescr() ثابت { واپس m_attrFileDescr; }
    bool attrSFormat() ثابت { واپس m_attrSFormat; }
    bool attr_بانٹو_Var() ثابت { واپس m_attr_بانٹو_Var; }
    bool attrFsmState() ثابت { واپس m_attrFsmState; }
    bool attrFsmArcInclCond() ثابت { واپس m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ثابت { واپس m_attrFsmStateExpand; }
    AstIface* sensIfacep() ثابت { واپس m_sensIfacep; }
    VRandAttr rand() ثابت { واپس m_rand; }
    string verilogKwd() ثابت override;
    خالی lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ثابت { واپس m_lifetime; }
    خالی pin_عدد(صحیح id) { m_pin_عدد = id; }
    صحیح pin_عدد() ثابت { واپس m_pin_عدد; }
    خالی propagateAttrFrom(ثابت AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        اگر (fromp->attrFileDescr()) attrFileDescr(true);
        اگر (fromp->isContinuously()) isContinuously(true);
    }
    خالی propagate_بانٹو_Attr_From(ثابت AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attr_بانٹو_Var(fromp->attr_بانٹو_Var());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ثابت حرف* cannot_بانٹو_قسم_Reason() ثابت;
    خالی propagateWrapAttrFrom(ثابت AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    خالی combineType(ثابت AstVar* otherp);
    bool needsCReset() ثابت {
        واپس !is_Iface_والد() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_عارضی()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    ساکن AstVar* scVarRecurse(Ast_گرہ* nodep);
};
class AstVarScope final : عوامی Ast_گرہ {
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
    bool m_fork_By_قدر : 1;  // Set before a fork, whose processes get it by value
عوامی:
    AstVarScope(File_سطر* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_قدر = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    خالی cloneRelink() override {
        اگر (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    string name() ثابت override VL_MT_STABLE { واپس scopep()->name() + "->" + varp()->name(); }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    AstVar* varp() ثابت VL_MT_STABLE { واپس m_varp; }  // [After Link] Pointer to variable
    خالی varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ثابت VL_MT_STABLE { واپس m_scopep; }  // Pointer to scope it's under
    خالی scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ثابت { واپس m_trace; }
    خالی trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ثابت { واپس m_optimizeLifePost; }
    خالی optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_قدر() ثابت { واپس m_fork_By_قدر; }
    خالی fork_By_قدر(bool flag) { m_fork_By_قدر = flag; }
};

// === AstNodeConsItem ===
class Ast_Cons_Array_شے final : عوامی Ast_گرہ_Cons_شے {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ثابت uint64_t m_اشاریہ;  // Index of the element, with the array's lo() at 0
عوامی:
    Ast_Cons_Array_شے(File_سطر* fl, uint64_t اشاریہ, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cons_Array_شے(fl, valuep)
        , m_اشاریہ{اشاریہ} {}
    ASTGEN_MEMBERS_Ast_Cons_Array_شے;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), Cons_Array_شے));
        واپس nullptr;
    }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        واپس m_اشاریہ == VN_DBG_AS(samep, Cons_Array_شے)->m_اشاریہ;
    }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }  // By AstConsArray's map
    uint64_t اشاریہ() ثابت { واپس m_اشاریہ; }
};
class Ast_Cons_Assoc_شے final : عوامی Ast_گرہ_Cons_شے {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
عوامی:
    Ast_Cons_Assoc_شے(File_سطر* fl, Ast_گرہ_اظہاریہ* keyp, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cons_Assoc_شے(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Assoc_شے;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), Cons_Assoc_شے));
        واپس nullptr;
    }
};
class Ast_Cons_Dyn_Array_شے final : عوامی Ast_گرہ_Cons_شے {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
عوامی:
    Ast_Cons_Dyn_Array_شے(File_سطر* fl, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_شے(fl, valuep) {}
    Ast_Cons_Dyn_Array_شے(File_سطر* fl, AstConst* keyp, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_شے(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Dyn_Array_شے;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), Cons_Dyn_Array_شے));
        واپس nullptr;
    }
};
class Ast_Cons_قطار_شے final : عوامی Ast_گرہ_Cons_شے {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
عوامی:
    Ast_Cons_قطار_شے(File_سطر* fl, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cons_قطار_شے(fl, valuep) {}
    ASTGEN_MEMBERS_Ast_Cons_قطار_شے;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), Cons_قطار) && !VN_IS(backp(), Cons_قطار_شے));
        واپس nullptr;
    }
};
class Ast_Cons_Struct_شے final : عوامی Ast_گرہ_Cons_شے {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
عوامی:
    Ast_Cons_Struct_شے(File_سطر* fl, AstMemberDType* memberp, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cons_Struct_شے(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_Ast_Cons_Struct_شے;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), Cons_Struct_شے));
        واپس nullptr;
    }
    خالی dump(std::ostream& str) ثابت override;
    AstMemberDType* memberp() ثابت { واپس m_memberp; }
};
class Ast_Cons_Wildcard_شے final : عوامی Ast_گرہ_Cons_شے {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
عوامی:
    Ast_Cons_Wildcard_شے(File_سطر* fl, AstConst* keyp, Ast_گرہ_اظہاریہ* valuep)
        : ASTGEN_SUPER_Cons_Wildcard_شے(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Wildcard_شے;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), Cons_Wildcard_شے));
        واپس nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : عوامی Ast_گرہ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsm_مقصد;  // FSM destination state label
    string m_fsm_لیبل;  // FSM arc kind tag (e.g. reset, reset_include, default)
    صحیح m_انحراف;  // Offset column numbers to uniq-ify IFs
عوامی:
    AstCoverOtherDecl(File_سطر* fl, ثابت string& صفحہ, ثابت string& comment,
                      ثابت string& linescov, صحیح انحراف, ثابت string& fsmVar = "",
                      ثابت string& fsmFrom = "", ثابت string& fsm_مقصد = "",
                      ثابت string& fsm_لیبل = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, صفحہ, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsm_مقصد{fsm_مقصد}
        , m_fsm_لیبل{fsm_لیبل}
        , m_انحراف{انحراف} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح انحراف() ثابت { واپس m_انحراف; }
    صحیح سائز() ثابت override { واپس 1; }
    ثابت string& linescov() ثابت { واپس m_linescov; }
    ثابت string& fsmVar() ثابت { واپس m_fsmVar; }
    ثابت string& fsmFrom() ثابت { واپس m_fsmFrom; }
    ثابت string& fsm_مقصد() ثابت { واپس m_fsm_مقصد; }
    ثابت string& fsm_لیبل() ثابت { واپس m_fsm_لیبل; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstCoverOtherDecl* ثابت asamep = VN_DBG_AS(samep, CoverOtherDecl);
        واپس Super::same_گرہ(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : عوامی Ast_گرہ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ثابت VNumRange m_range;  // Packed array range covering each toggle bit
عوامی:
    AstCoverToggleDecl(File_سطر* fl, ثابت string& صفحہ, ثابت string& comment,
                       ثابت VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, صفحہ, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    صحیح سائز() ثابت override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        واپس 2 * m_range.elements();
    }
    ثابت VNumRange& range() ثابت { واپس m_range; }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstCoverToggleDecl* ثابت asamep = VN_DBG_AS(samep, CoverToggleDecl);
        واپس Super::same_گرہ(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : عوامی Ast_گرہ_FTask {
    // A function inside a module
عوامی:
    AstFunc(File_سطر* fl, ثابت string& name, Ast_گرہ* stmtp, Ast_گرہ* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    Ast_گرہ_FTask* cloneType(ثابت string& name) override {
        واپس new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ثابت override { واپس "function"; }
};
class AstLet final : عوامی Ast_گرہ_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
عوامی:
    AstLet(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_اظہاریہ));
        واپس nullptr;
    }
    Ast_گرہ_FTask* cloneType(ثابت string& name) override { واپس new AstLet{fileline(), name}; }
    string verilogKwd() ثابت override { واپس "let"; }
};
class AstProperty final : عوامی Ast_گرہ_FTask {
    // A property inside a module
عوامی:
    AstProperty(File_سطر* fl, ثابت string& name, Ast_گرہ* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    Ast_گرہ_FTask* cloneType(ثابت string& name) override {
        واپس new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { واپس "property"; }
};
class AstSequence final : عوامی Ast_گرہ_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
عوامی:
    AstSequence(File_سطر* fl, ثابت string& name, Ast_گرہ* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { واپس true; }
    Ast_گرہ_FTask* cloneType(ثابت string& name) override {
        واپس new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { واپس "sequence"; }
    bool isReferenced() ثابت { واپس m_referenced; }
    خالی isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : عوامی Ast_گرہ_FTask {
    // A task inside a module
عوامی:
    AstTask(File_سطر* fl, ثابت string& name, Ast_گرہ* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_گرہ_FTask* cloneType(ثابت string& name) override {
        واپس new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { واپس "task"; }
};

// === AstNodeFile ===
class AstCFile final : عوامی Ast_گرہ_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_ماخذ : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
عوامی:
    AstCFile(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_ماخذ{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
    uint64_t complexityScore() ثابت { واپس m_complexityScore; }
    خالی complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ثابت { واپس m_slow; }
    خالی slow(bool flag) { m_slow = flag; }
    bool ماخذ() ثابت { واپس m_ماخذ; }
    خالی ماخذ(bool flag) { m_ماخذ = flag; }
    bool support() ثابت { واپس m_support; }
    خالی support(bool flag) VL_MT_محفوظ { m_support = flag; }
};
class AstVFile final : عوامی Ast_گرہ_File {
    // Verilog output file
    // Parents:  NETLIST
عوامی:
    AstVFile(File_سطر* fl, ثابت string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    خالی dump(std::ostream& str = std::cout) ثابت override;
    خالی dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstCoverCross final : عوامی Ast_گرہ_Func_Cov_شے {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
عوامی:
    AstCoverCross(File_سطر* fl, ثابت string& name, AstCoverpointRef* itemsp,
                  Ast_گرہ_اظہاریہ* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        جوڑو_Itemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class AstCoverpoint final : عوامی Ast_گرہ_Func_Cov_شے {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
عوامی:
    AstCoverpoint(File_سطر* fl, ثابت string& name, Ast_گرہ_اظہاریہ* exprp,
                  Ast_گرہ_اظہاریہ* iffp = nullptr, Ast_گرہ* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        جوڑو_Binsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};

// === AstNodeGen ===
class Ast_Gen_بلاک final : عوامی Ast_گرہ_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ثابت bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ثابت bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

عوامی:
    Ast_Gen_بلاک(File_سطر* fl, ثابت string& name, Ast_گرہ* itemsp, bool implied)
        : ASTGEN_SUPER_Gen_بلاک(fl)
        , m_name{name}
        , m_unnamed{name.خالی_خانے()}
        , m_implied{implied} {
        this->جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_بلاک;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    std::string name() ثابت override VL_MT_STABLE { واپس m_name; }
    خالی name(ثابت std::string& name) override { m_name = name; }
    bool unnamed() ثابت { واپس m_unnamed; }
    bool implied() ثابت { واپس m_implied; }
    AstDefaultDisable* defaultDisablep() ثابت { واپس m_defaultDisablep; }
    خالی defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ثابت string& outer) ثابت;
};
class AstGenCase final : عوامی Ast_گرہ_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
عوامی:
    AstGenCase(File_سطر* fl, Ast_گرہ_اظہاریہ* exprp, Ast_Gen_Case_شے* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : عوامی Ast_گرہ_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
عوامی:
    AstGenFor(File_سطر* fl, Ast_گرہ* initsp, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* incsp, Ast_گرہ* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->جوڑو_Initsp(initsp);
        this->condp(condp);
        this->جوڑو_Incsp(incsp);
        this->جوڑو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : عوامی Ast_گرہ_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
عوامی:
    AstGenIf(File_سطر* fl, Ast_گرہ_اظہاریہ* condp, Ast_گرہ* thensp, Ast_گرہ* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->جوڑو_Thensp(thensp);
        this->جوڑو_Elsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : عوامی Ast_گرہ_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtype_Name_بھرا;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_عدد;  // Declaration token number
    VBaseOverride m_بنیاد_Override;  // BaseOverride (inital/final/extends)
    bool m_hasRandVarsUpdate = false;  // Has updateRandVars method,
                                       // which updates pointers to rand variables in clone()
    bool m_covergroup = false;  // Is covergroup (TODO perhaps make a new Ast node type for CG?)
    bool m_extended = false;  // Is extension or extended by other classes
    bool m_interfaceClass = false;  // Interface class
    bool m_needRNG = false;  // Need RNG, uses srandom/randomize
    bool m_use_Virtual_عوامی = false;  // Subclasses need virtual public as uses interface class
    bool m_virtual = false;  // Virtual class
    bool m_printedFrom = false;  // This class is printed from i.e. is used as format arg.
    // Mutable, as only a recursion guard of the const dtypeName(), so also not dumped
    mutable bool m_dtypeNameBusy = false;  // In dtypeName(), which a parameter may lead back to
    // Covergroup options (when m_covergroup is true)
    صحیح m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool بھرا_2, ثابت string& scope) ثابت;
    string dtypeNameScope() ثابت;  // Prefix of dtypeName(true) for the scope declaring the class

عوامی:
    AstClass(File_سطر* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_عدد{fl->token_عدد()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ثابت override { واپس isCovergroup() ? "covergroup" : "class"; }
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس true; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    bool timescaleMatters() ثابت override { واپس false; }
    AstClassPackage* classOrPackagep() ثابت VL_MT_STABLE { واپس m_classOrPackagep; }
    خالی classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ثابت VL_MT_STABLE {
        واپس m_covergroupEnclosingClassp;
    }
    خالی covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_گرہ* membersp() ثابت VL_MT_STABLE { واپس stmtsp(); }
    خالی جوڑو_Membersp(Ast_گرہ* nodep) { جوڑو_Stmtsp(nodep); }
    bool hasRandVarsUpdate() ثابت { واپس m_hasRandVarsUpdate; }
    خالی hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ثابت { واپس m_covergroup; }
    خالی isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ثابت { واپس m_extended; }
    خالی isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ثابت { واپس m_interfaceClass; }
    خالی isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ثابت { واپس m_virtual; }
    خالی isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ثابت { واپس m_needRNG; }
    خالی needRNG(bool flag) { m_needRNG = flag; }
    bool use_Virtual_عوامی() ثابت { واپس m_use_Virtual_عوامی; }
    خالی use_Virtual_عوامی(bool flag) { m_use_Virtual_عوامی = flag; }
    خالی markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ثابت { واپس m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool بھرا_2) ثابت;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtype_Name_اندر(ثابت string& scope) ثابت;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    خالی dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ثابت { واپس !m_dtype_Name_بھرا.خالی_خانے(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_پیغام() ثابت override { واپس hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    ساکن string embeddedCovergroupTypeName(ثابت string& name) { واپس "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ثابت;
    // Covergroup options accessors
    صحیح cgAutoBinMax() ثابت { واپس m_cgAutoBinMax; }
    خالی cgAutoBinMax(صحیح قدر) { m_cgAutoBinMax = قدر; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    ساکن bool isClassExtendedFrom(ثابت AstClass* refClassp, ثابت AstClass* بنیاد_Classp);
    uint32_t decl_Token_عدد() ثابت override { واپس m_decl_Token_عدد; }
    خالی decl_Token_عدد_سیٹ_Min(uint32_t token_عدد) override {
        m_decl_Token_عدد = std::min(m_decl_Token_عدد, token_عدد);
    }
    خالی بنیاد_Override(ثابت VBaseOverride& flag) { m_بنیاد_Override = flag; }
    VBaseOverride بنیاد_Override() ثابت { واپس m_بنیاد_Override; }
    // Return the lowest class extended from, or this class
    AstClass* بنیاد_Most_Classp();
    ساکن bool is_Cacheable_بچہ(ثابت Ast_گرہ* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    سانچہ <typename T_Callable>
    خالی foreachMember(ثابت T_Callable& f) {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_گرہ*>::قدر
                && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        اگر (ثابت AstClassExtends* ثابت cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        برائے (Ast_گرہ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (Ast_گرہ::نجی_Type_Test<T_گرہ>(stmtp)) f(this, static_cast<T_گرہ*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    سانچہ <typename T_Callable>
    bool existsMember(ثابت T_Callable& p) ثابت {
        using T_گرہ = typename Function_Arg_No_اشارہ_گر_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ثابت AstClass*, ثابت T_گرہ*>::قدر
                && std::is_بنیاد_کا<Ast_گرہ, T_گرہ>::قدر,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        اگر (ثابت AstClassExtends* ثابت cextendsp = this->extendsp()) {
            اگر (cextendsp->classp()->existsMember(p)) واپس true;
        }
        برائے (Ast_گرہ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            اگر (Ast_گرہ::نجی_Type_Test<T_گرہ>(stmtp)) {
                اگر (p(this, static_cast<T_گرہ*>(stmtp))) واپس true;
            }
        }
        واپس false;
    }
};
class AstClassPackage final : عوامی Ast_گرہ_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
عوامی:
    AstClassPackage(File_سطر* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ثابت override { واپس "classpackage"; }
    bool timescaleMatters() ثابت override { واپس false; }
    AstClass* classp() ثابت VL_MT_محفوظ { واپس m_classp; }
    خالی classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : عوامی Ast_گرہ_Module {
    // An interface declaration
    string m_dtype_Name_بھرا;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
عوامی:
    AstIface(File_سطر* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { واپس "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ثابت override { واپس false; }
    bool hasVirtualRef() ثابت { واپس m_hasVirtualRef; }
    خالی سیٹ_Has_Virtual_Ref() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool بھرا_2) ثابت;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    خالی dtypeNameFreeze();
};
class AstModule final : عوامی Ast_گرہ_Module {
    // A module declaration
    ثابت bool m_isChecker;  // Module represents a checker
    ثابت bool m_is_پروگرام;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

عوامی:
    class Checker {};  // for constructor type-overload selection
    class پروگرام {};  // for constructor type-overload selection
    AstModule(File_سطر* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_پروگرام{false} {}
    AstModule(File_سطر* fl, ثابت string& name, ثابت string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_is_پروگرام{false} {}
    AstModule(File_سطر* fl, ثابت string& name, ثابت string& libname, پروگرام)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_پروگرام{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ثابت override {
        واپس m_isChecker ? "checker" : m_is_پروگرام ? "program" : "module";
    }
    bool timescaleMatters() ثابت override { واپس true; }
    bool isChecker() ثابت { واپس m_isChecker; }
    bool is_پروگرام() ثابت { واپس m_is_پروگرام; }
    bool hasGenericIface() ثابت { واپس m_hasGenericIface; }
    خالی hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
};
class Ast_Not_ملا_Module final : عوامی Ast_گرہ_Module {
    // A missing module declaration
عوامی:
    Ast_Not_ملا_Module(File_سطر* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Not_ملا_Module(fl, name, libname) {}
    ASTGEN_MEMBERS_Ast_Not_ملا_Module;
    string verilogKwd() ثابت override { واپس "/*not-found-*/ module"; }
    bool timescaleMatters() ثابت override { واپس false; }
};
class AstPackage final : عوامی Ast_گرہ_Module {
    // A package declaration
عوامی:
    AstPackage(File_سطر* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ثابت override { واپس "package"; }
    bool timescaleMatters() ثابت override { واپس !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : عوامی Ast_گرہ_Module {
    // A primitive declaration
عوامی:
    AstPrimitive(File_سطر* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ثابت override { واپس "primitive"; }
    bool timescaleMatters() ثابت override { واپس false; }
};

// === AstNodeProcedure ===
class AstAlways final : عوامی Ast_گرہ_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ثابت VAlwaysKwd m_keyword;

عوامی:
    AstAlways(File_سطر* fl, VAlwaysKwd keyword, Ast_Sen_درخت* sentreep, Ast_گرہ* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit اندرونی AstAlways(Ast_تفویض_W* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    VAlwaysKwd keyword() ثابت { واپس m_keyword; }
};
class AstAlwaysObserved final : عوامی Ast_گرہ_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

عوامی:
    AstAlwaysObserved(File_سطر* fl, Ast_Sen_درخت* sentreep, Ast_گرہ* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : عوامی Ast_گرہ_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

عوامی:
    explicit AstAlwaysPost(File_سطر* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : عوامی Ast_گرہ_Procedure {
    // Like always but Postponed scheduling region

عوامی:
    AstAlwaysPostponed(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : عوامی Ast_گرہ_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

عوامی:
    explicit AstAlwaysPre(File_سطر* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : عوامی Ast_گرہ_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

عوامی:
    AstAlwaysReactive(File_سطر* fl, Ast_Sen_درخت* sentreep, Ast_گرہ* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : عوامی Ast_گرہ_Procedure {
عوامی:
    AstFinal(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : عوامی Ast_گرہ_Procedure {
عوامی:
    AstInitial(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : عوامی Ast_گرہ_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
عوامی:
    AstInitialAutomatic(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : عوامی Ast_گرہ_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
عوامی:
    AstInitialStatic(File_سطر* fl, Ast_گرہ* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : عوامی Ast_گرہ_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
عوامی:
    AstBracketRange(File_سطر* fl, Ast_گرہ* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    مجازی string emitC() { V3ERROR_NA_RETURN(""); }
    مجازی string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_مقصد() ثابت override VL_MT_محفوظ { واپس false; }
};
class AstRange final : عوامی Ast_گرہ_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ثابت bool m_fromBracket = false;  // From C-style '[x]' declaration
عوامی:
    AstRange(File_سطر* fl, Ast_گرہ_اظہاریہ* leftp, Ast_گرہ_اظہاریہ* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    اندرونی AstRange(File_سطر* fl, صحیح بایاں, صحیح دایاں);
    اندرونی AstRange(File_سطر* fl, ثابت VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    اندرونی صحیح بایاں_Const() ثابت VL_MT_STABLE;
    اندرونی صحیح دایاں_Const() ثابت VL_MT_STABLE;
    صحیح hiConst() ثابت VL_MT_STABLE {
        ثابت صحیح l = بایاں_Const();
        ثابت صحیح r = دایاں_Const();
        واپس l > r ? l : r;
    }
    صحیح loConst() ثابت VL_MT_STABLE {
        ثابت صحیح l = بایاں_Const();
        ثابت صحیح r = دایاں_Const();
        واپس l > r ? r : l;
    }
    صحیح elementsConst() ثابت VL_MT_STABLE { واپس hiConst() - loConst() + 1; }
    bool ascending() ثابت { واپس بایاں_Const() < دایاں_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_بایاں_مقصد_دایاں() ثابت {
        واپس VNumRange{بایاں_Const(), دایاں_Const()}.seq_بایاں_مقصد_دایاں();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_دایاں_مقصد_بایاں() ثابت {
        واپس VNumRange{بایاں_Const(), دایاں_Const()}.seq_دایاں_مقصد_بایاں();
    }
    خالی dump(std::ostream& str) ثابت override;
    خالی dumpJson(std::ostream& str) ثابت override;
    مجازی string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_گرہ(ثابت Ast_گرہ* samep) ثابت override {
        ثابت AstRange* ثابت asamep = VN_DBG_AS(samep, Range);
        واپس fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ثابت { واپس m_fromBracket; }
};
class AstUnsizedRange final : عوامی Ast_گرہ_Range {
    // Unsized range specification, for open arrays
عوامی:
    explicit AstUnsizedRange(File_سطر* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    مجازی string emitC() { V3ERROR_NA_RETURN(""); }
    مجازی string emitVerilog() { واپس "[]"; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};
class AstWildcardRange final : عوامی Ast_گرہ_Range {
    // Wildcard range specification, for wildcard index type associative arrays
عوامی:
    explicit AstWildcardRange(File_سطر* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    مجازی string emitC() { V3ERROR_NA_RETURN(""); }
    مجازی string emitVerilog() { واپس "[*]"; }
    bool same_گرہ(ثابت Ast_گرہ* /*samep*/) ثابت override { واپس true; }
};

#اگر_ختم  // Guard
