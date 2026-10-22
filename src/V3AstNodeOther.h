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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ASTNODES_H_
#وصف_ڪريو VERILATOR_V3ASTNODES_H_

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3AST_H_
#غلطي "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#وصف_ڪريو VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#جيڪڏهن_پورو

#ضمن <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_ڳنڍ_Cons_شي VL_NOT_FINAL : عام Ast_ڳنڍ {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    دوست class Ast_ڳنڍ_اظهار;  // For m_knownConstantTree
    mutable bool m_معلوم_Constant_وڻ = false;  // Known to be a tree of constants

محمي:
    Ast_ڳنڍ_Cons_شي(VNType t, File_سٽ* fl, Ast_ڳنڍ_اظهار* valuep)
        : Ast_ڳنڍ{t, fl} {
        this->valuep(valuep);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Cons_شي;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class Ast_ڳنڍ_Cover_Decl VL_NOT_FINAL : عام Ast_ڳنڍ {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_صفحو;  // Coverage point's page tag
    string m_متن;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    صحيح m_bin_انگ = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    صحيح m_local_Bin_انگ = 0;  // Per-module coverage bin offset
    bool m_في_Instance = false;  // Don't clone during inlining, already per instance
عام:
    Ast_ڳنڍ_Cover_Decl(VNType t, File_سٽ* fl, ثابت string& صفحو, ثابت string& comment)
        : Ast_ڳنڍ(t, fl)
        , m_صفحو{صفحو}
        , m_متن{comment} {}
    ASTGEN_MEMBERS_Ast_ڳنڍ_Cover_Decl;
    ثابت حرف* broken() ثابت override {
        إذا (m_معلومات_Declp
            && (m_معلومات_Declp == this || m_معلومات_Declp->m_معلومات_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvt_منزل_Hex(this));
        }
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_ڳڻپ() ثابت override { أرجع 1 + 2 * INSTR_ڳڻپ_LD; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    صحيح bin_انگ() ثابت { أرجع m_bin_انگ; }
    فراغ bin_انگ(صحيح flag) { m_bin_انگ = flag; }
    صحيح local_Bin_انگ() ثابت { أرجع m_local_Bin_انگ; }
    فراغ local_Bin_انگ(صحيح flag) { m_local_Bin_انگ = flag; }
    bool في_Instance() ثابت { أرجع m_في_Instance; }
    فراغ في_Instance(bool flag) { m_في_Instance = flag; }
    مجازي صحيح سائيز() ثابت = 0;
    ثابت string& comment() ثابت { أرجع m_متن; }  // text to insert in code
    ثابت string& صفحو() ثابت { أرجع m_صفحو; }
    ثابت string& hier() ثابت { أرجع m_hier; }
    فراغ hier(ثابت string& flag) { m_hier = flag; }
    فراغ comment(ثابت string& flag) { m_متن = flag; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت Ast_ڳنڍ_Cover_Decl* ثابت asamep = VN_DBG_AS(samep, ڳنڍ_Cover_Decl);
        أرجع (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && صفحو() == asamep->صفحو()
                && في_Instance() == asamep->في_Instance());
    }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    فراغ معلومات_Declp(Ast_ڳنڍ_Cover_Decl* nodep) { m_معلومات_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_ڳنڍ_Cover_Decl* معلومات_Decl_Nullp() ثابت { أرجع m_معلومات_Declp; }
    Ast_ڳنڍ_Cover_Decl* معلومات_Decl_Thisp() { أرجع معلومات_Decl_Nullp() ? معلومات_Decl_Nullp() : this; }
};
class Ast_ڳنڍ_FTask VL_NOT_FINAL : عام Ast_ڳنڍ {
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
    string m_iface_پورٽ_Name;  // Interface port name for out-of-block definition (IEEE 25.8)
    uint64_t m_dpi_Open_والد = 0;  // DPI import open array, if !=0, how many callees
    bool m_task_عوامي : 1;  // Public task
    bool m_class_طريقو : 1;  // Class method
    bool m_didProto : 1;  // Did prototype processing
    bool m_prototype : 1;  // Just a prototype
    bool m_dpiExport : 1;  // DPI exported
    bool m_dpiImport : 1;  // DPI imported
    bool m_dpiContext : 1;  // DPI import context
    bool m_dpi_Open_ٻار : 1;  // DPI import open array child wrapper
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
    VBaseOverride m_بنياد_Override;  // BaseOverride (inital/final/extends)
    VLifetime m_lifetime;  // Default lifetime of local vars
    VIsCached m_purity;  // Pure state

محمي:
    Ast_ڳنڍ_FTask(VNType t, File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* stmtsp)
        : Ast_ڳنڍ{t, fl}
        , m_name{name}
        , m_task_عوامي{false}
        , m_class_طريقو{false}
        , m_didProto{false}
        , m_prototype{false}
        , m_dpiExport{false}
        , m_dpiImport{false}
        , m_dpiContext{false}
        , m_dpi_Open_ٻار{false}
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
        جوڙيو_Stmtsp(stmtsp);
        cname(name);  // Might be overridden by dpi import/export
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_FTask;
    مجازي Ast_ڳنڍ_FTask* cloneType(ثابت string& name) = 0;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    bool isGateOptimizable() ثابت override {
        أرجع !((m_dpiExport || m_dpiImport) && !m_dpi_خالص);
    }
    // {AstFunc only} op1 = Range output variable
    فراغ name(ثابت string& name) override { m_name = name; }
    string cname() ثابت { أرجع m_cname; }
    فراغ cname(ثابت string& cname) { m_cname = cname; }
    bool isFunction() ثابت { أرجع fvarp() != nullptr; }
    // MORE ACCESSORS
    فراغ dpi_Open_والد_Inc() { ++m_dpi_Open_والد; }
    فراغ dpi_Open_والد_Clear() { m_dpi_Open_والد = 0; }
    uint64_t dpi_Open_والد() ثابت { أرجع m_dpi_Open_والد; }
    bool task_عوامي() ثابت { أرجع m_task_عوامي; }
    فراغ task_عوامي(bool flag) { m_task_عوامي = flag; }
    bool class_طريقو() ثابت { أرجع m_class_طريقو; }
    فراغ class_طريقو(bool flag) { m_class_طريقو = flag; }
    bool didProto() ثابت { أرجع m_didProto; }
    فراغ didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ثابت { أرجع m_isExternProto; }
    فراغ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ثابت { أرجع m_isExternDef; }
    فراغ isExternDef(bool flag) { m_isExternDef = flag; }
    ثابت string& iface_پورٽ_Name() ثابت { أرجع m_iface_پورٽ_Name; }
    فراغ iface_پورٽ_Name(ثابت string& name) { m_iface_پورٽ_Name = name; }
    bool prototype() ثابت { أرجع m_prototype; }
    فراغ prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ثابت { أرجع m_dpiExport; }
    فراغ dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ثابت { أرجع m_dpiImport; }
    فراغ dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ثابت { أرجع m_dpiContext; }
    فراغ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpi_Open_ٻار() ثابت { أرجع m_dpi_Open_ٻار; }
    فراغ dpi_Open_ٻار(bool flag) { m_dpi_Open_ٻار = flag; }
    bool dpiTask() ثابت { أرجع m_dpiTask; }
    فراغ dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ثابت { أرجع !m_dpiCDecl.خالي_جايون(); }
    ثابت string& dpiCDecl() ثابت { أرجع m_dpiCDecl; }
    فراغ dpiCDecl(ثابت string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ثابت { أرجع m_isConstructor; }
    فراغ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ثابت { أرجع m_isHideLocal; }
    فراغ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ثابت { أرجع m_isHideProtected; }
    فراغ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpi_خالص() ثابت { أرجع m_dpi_خالص; }
    فراغ dpi_خالص(bool flag) { m_dpi_خالص = flag; }
    bool keepAlive() ثابت { أرجع m_keepAlive; }
    فراغ keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ثابت { أرجع m_needsSyms; }
    فراغ needsSyms(bool flag) { m_needsSyms = flag; }
    bool خالص_Virtual() ثابت { أرجع m_خالص_Virtual; }
    فراغ خالص_Virtual(bool flag) { m_خالص_Virtual = flag; }
    bool recursive() ثابت { أرجع m_recursive; }
    فراغ recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ثابت { أرجع m_static; }
    فراغ isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ثابت { أرجع m_underGenerate; }
    فراغ underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ثابت { أرجع m_verilogFunction; }
    فراغ verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ثابت { أرجع m_verilogTask; }
    فراغ verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ثابت { أرجع m_virtual; }
    فراغ isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ثابت { أرجع m_needProcess; }
    فراغ سيٽ_Need_Process() { m_needProcess = true; }
    bool isCovergroupSample() ثابت { أرجع m_isCovergroupSample; }
    فراغ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    فراغ بنياد_Override(ثابت VBaseOverride& flag) { m_بنياد_Override = flag; }
    VBaseOverride بنياد_Override() ثابت { أرجع m_بنياد_Override; }
    فراغ lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ثابت { أرجع m_lifetime; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;
    فراغ propagateAttrFrom(ثابت Ast_ڳنڍ_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        class_طريقو(fromp->class_طريقو());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

خاص:
    bool getPurityRecurse() ثابت;
};
class Ast_ڳنڍ_File VL_NOT_FINAL : عام Ast_ڳنڍ {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
عام:
    Ast_ڳنڍ_File(VNType t, File_سٽ* fl, ثابت string& name)
        : Ast_ڳنڍ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ڳنڍ_File;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_ڳنڍ_Func_Cov_شي VL_NOT_FINAL : عام Ast_ڳنڍ {
    // Base class for functional coverage items (coverpoints, crosses)
محمي:
    string m_name;  // Item name

عام:
    Ast_ڳنڍ_Func_Cov_شي(VNType t, File_سٽ* fl, ثابت string& name)
        : Ast_ڳنڍ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ڳنڍ_Func_Cov_شي;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    bool maybe_Pointed_منزل() ثابت override { أرجع true; }
};
class Ast_ڳنڍ_Gen VL_NOT_FINAL : عام Ast_ڳنڍ {
    // Generate construct
عام:
    Ast_ڳنڍ_Gen(VNType t, File_سٽ* fl)
        : Ast_ڳنڍ{t, fl} {}
    ASTGEN_MEMBERS_Ast_ڳنڍ_Gen;
};
class Ast_ڳنڍ_Module VL_NOT_FINAL : عام Ast_ڳنڍ {
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
    صحيح m_اونهائي = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    صحيح m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
    VLifetime m_lifetime;  // Lifetime
    VTimescale m_timeunit;  // Global time unit
    VOptionBool m_unconnectedDrive;  // State of `unconnected_drive
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

    bool m_mod_عوامي : 1;  // Module has public references
    bool m_modTrace : 1;  // Tracing this module
    bool m_اندريون_Library : 1;  // From a library, no error if not used, never top level
    bool m_ctorVarReset : 1;  // Ctor needs to call ctor_var_reset
    bool m_dead : 1;  // LinkDot believes is dead; will remove in Dead visitors
    bool m_hasGParam : 1;  // Has global parameter (for link)
    bool m_hasParameterList : 1;  // Has #() for parameter declaration
    bool m_hier_بلاڪ : 1;  // Hierarchical Block marked by HIER_BLOCK pragma
    bool m_hierParams : 1;  // Block containing params for parameterized hier blocks
    bool m_recursive : 1;  // Recursive module
    bool m_recursiveClone : 1;  // If recursive, what module it clones, otherwise nullptr
    bool m_parameterizedTemplate : 1;  // True when at least one specialized clone exists;
                                       // set by V3Param::deepCloneModule. Suppresses
                                       // width/type errors on the unresolved template.
    bool m_verilatorLib : 1;  // Module is a stub for a Verilator produced --lib-create
محمي:
    Ast_ڳنڍ_Module(VNType t, File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : Ast_ڳنڍ{t, fl}
        , m_name{name}
        , m_origName{name}
        , m_libname{libname}
        , m_mod_عوامي{false}
        , m_modTrace{false}
        , m_اندريون_Library{false}
        , m_ctorVarReset{false}
        , m_dead{false}
        , m_hasGParam{false}
        , m_hasParameterList{false}
        , m_hier_بلاڪ{false}
        , m_hierParams{false}
        , m_recursive{false}
        , m_recursiveClone{false}
        , m_parameterizedTemplate{false}
        , m_verilatorLib{false} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Module;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    مجازي bool timescaleMatters() ثابت = 0;
    مضمن bool isDollarUnit() ثابت;  // Is the $unit package
    مضمن bool isConstPool() ثابت;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ثابت string& outer) ثابت;
    // ACCESSORS
    فراغ name(ثابت string& name) override { m_name = name; }
    string origName() ثابت override { أرجع m_origName; }
    string someInstanceName() ثابت VL_MT_محفوظ { أرجع m_someInstanceName; }
    فراغ someInstanceName(ثابت string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ثابت { أرجع m_defaultDisablep; }
    فراغ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool اندريون_Library() ثابت { أرجع m_اندريون_Library; }
    فراغ اندريون_Library(bool flag) { m_اندريون_Library = flag; }
    فراغ اونهائي(صحيح قدر) { m_اونهائي = قدر; }
    صحيح اونهائي() ثابت VL_MT_محفوظ { أرجع m_اونهائي; }
    فراغ level(صحيح قدر) { m_level = قدر; }
    صحيح level() ثابت VL_MT_محفوظ { أرجع m_level; }
    string libname() ثابت { أرجع m_libname; }
    bool is_چوٽي() ثابت VL_MT_محفوظ { أرجع level() == 1; }
    bool mod_عوامي() ثابت { أرجع m_mod_عوامي; }
    فراغ mod_عوامي(bool flag) { m_mod_عوامي = flag; }
    bool modTrace() ثابت { أرجع m_modTrace; }
    فراغ modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ثابت { أرجع m_ctorVarReset; }
    فراغ ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ثابت { أرجع m_dead; }
    فراغ dead(bool flag) { m_dead = flag; }
    bool hasGParam() ثابت { أرجع m_hasGParam; }
    فراغ hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ثابت { أرجع m_hasParameterList; }
    فراغ hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hier_بلاڪ() ثابت { أرجع m_hier_بلاڪ; }
    فراغ hier_بلاڪ(bool flag) { m_hier_بلاڪ = flag; }
    bool hierParams() ثابت { أرجع m_hierParams; }
    فراغ hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ثابت { أرجع m_recursive; }
    فراغ recursive(bool flag) { m_recursive = flag; }
    فراغ recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ثابت { أرجع m_recursiveClone; }
    bool parameterizedTemplate() ثابت { أرجع m_parameterizedTemplate; }
    فراغ parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    فراغ verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ثابت { أرجع m_verilatorLib; }
    VLifetime lifetime() ثابت { أرجع m_lifetime; }
    فراغ lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ثابت { أرجع m_unconnectedDrive; }
    فراغ unconnectedDrive(ثابت VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_ڳنڍ_Procedure VL_NOT_FINAL : عام Ast_ڳنڍ {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
محمي:
    Ast_ڳنڍ_Procedure(VNType t, File_سٽ* fl, Ast_ڳنڍ* stmtsp)
        : Ast_ڳنڍ{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        جوڙيو_Stmtsp(stmtsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Procedure;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isJustOneBodyStmt() ثابت { أرجع stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ثابت { أرجع m_suspendable; }
    فراغ سيٽ_Suspendable() { m_suspendable = true; }
    bool needProcess() ثابت { أرجع m_needProcess; }
    فراغ سيٽ_Need_Process() { m_needProcess = true; }
};
class Ast_ڳنڍ_Range VL_NOT_FINAL : عام Ast_ڳنڍ {
    // A range, sized or unsized
محمي:
    Ast_ڳنڍ_Range(VNType t, File_سٽ* fl)
        : Ast_ڳنڍ{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_ڳنڍ_Range;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_سرگرم final : عام Ast_ڳنڍ {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

عام:
    Ast_سرگرم(File_سٽ* fl, ثابت string& name, Ast_Sen_وڻ* sentreep)
        : ASTGEN_SUPER_سرگرم(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_Ast_سرگرم;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    // Statements are broken into pieces, as some must come before others.
    فراغ sentreep(Ast_Sen_وڻ* nodep) { m_sentreep = nodep; }
    Ast_Sen_وڻ* sentreep() ثابت { أرجع m_sentreep; }
    // METHODS
    مضمن bool hasClocked() ثابت;
    مضمن bool hasCombo() ثابت;
    مضمن bool hasInitial() ثابت;
    مضمن bool hasStatic() ثابت;
};
class AstAlias final : عام Ast_ڳنڍ {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
عام:
    AstAlias(File_سٽ* fl, Ast_ڳنڍ_اظهار* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : عام Ast_ڳنڍ {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
عام:
    AstAliasScope(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : عام Ast_ڳنڍ {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
عام:
    AstBind(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        جوڙيو_Cellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Bind Target name
    فراغ name(ثابت string& name) override { m_name = name; }
};
class AstCFunc final : عام Ast_ڳنڍ {
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
    bool m_decl_ذاتي : 1;  // Declare it private
    bool m_keep_If_خالي_جايون : 1;  // Keep declaration and definition separate, even if empty
    bool m_slow : 1;  // Slow routine, called once or just at init time
    bool m_func_عوامي : 1;  // From user public task/function
    bool m_isConstructor : 1;  // Is C class constructor
    bool m_isDestructor : 1;  // Is C class destructor
    bool m_is_طريقو : 1;  // Is inside a class definition
    bool m_isLoose : 1;  // Semantically this is a method, but is implemented as a function with
                         // an explicitly passed 'self' pointer as the first argument.  This can
                         // be slightly faster due to __restrict, and we do not declare in header
                         // so adding/removing loose functions doesn't recompile everything.
    bool m_isVirtual : 1;  // Virtual function
    bool m_داخلا_نقطو : 1;  // User may call into this top level function
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
    صحيح m_لاڳت;  // Function call cost
عام:
    AstCFunc(File_سٽ* fl, ثابت string& name, AstScope* scopep, ثابت string& rtnType = "")
        : ASTGEN_SUPER_CFunc(fl) {
        m_isConst = VBoolOrUnknown::BU_UNKNOWN;  // Unknown until analyzed
        m_scopep = scopep;
        m_name = name;
        m_rtnType = rtnType;
        m_isStatic = false;
        m_isTrace = false;
        m_dontCombine = false;
        m_decl_ذاتي = false;
        m_keep_If_خالي_جايون = false;
        m_slow = false;
        m_func_عوامي = false;
        m_isConstructor = false;
        m_isDestructor = false;
        m_is_طريقو = true;
        m_isLoose = false;
        m_isVirtual = false;
        m_needProcess = false;
        m_داخلا_نقطو = false;
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
        m_لاڳت = v3Global.opt.instr_ڳڻپ_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstCFunc* ثابت asamep = VN_DBG_AS(samep, CFunc);
        أرجع ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    فراغ name(ثابت string& name) override { m_name = name; }
    صحيح instr_ڳڻپ() ثابت override { أرجع m_لاڳت; }
    VBoolOrUnknown isConst() ثابت { أرجع m_isConst; }
    فراغ isConst(bool flag) { m_isConst.سيٽ_True_Or_False(flag); }
    فراغ isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ثابت { أرجع m_isStatic; }
    فراغ isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ثابت VL_MT_محفوظ { أرجع m_isTrace; }
    فراغ isTrace(bool flag) { m_isTrace = flag; }
    فراغ cname(ثابت string& name) { m_cname = name; }
    string cname() ثابت { أرجع m_cname; }
    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ثابت { أرجع ((m_rtnType == "") ? "void" : m_rtnType); }
    فراغ rtnType(ثابت string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ثابت { أرجع m_dontCombine || isTrace() || داخلا_نقطو(); }
    فراغ dontCombine(bool flag) { m_dontCombine = flag; }
    bool decl_ذاتي() ثابت { أرجع m_decl_ذاتي; }
    فراغ decl_ذاتي(bool flag) { m_decl_ذاتي = flag; }
    bool keep_If_خالي_جايون() ثابت VL_MT_محفوظ { أرجع m_keep_If_خالي_جايون; }
    فراغ keep_If_خالي_جايون(bool flag) { m_keep_If_خالي_جايون = flag; }
    bool slow() ثابت VL_MT_محفوظ { أرجع m_slow; }
    فراغ slow(bool flag) { m_slow = flag; }
    bool func_عوامي() ثابت { أرجع m_func_عوامي; }
    فراغ func_عوامي(bool flag) { m_func_عوامي = flag; }
    فراغ argTypes(ثابت string& str) { m_argTypes = str; }
    string argTypes() ثابت { أرجع m_argTypes; }
    فراغ ifdef(ثابت string& str) { m_ifdef = str; }
    string ifdef() ثابت { أرجع m_ifdef; }
    bool isConstructor() ثابت { أرجع m_isConstructor; }
    فراغ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ثابت { أرجع m_isDestructor; }
    فراغ isDestructor(bool flag) { m_isDestructor = flag; }
    bool is_طريقو() ثابت { أرجع m_is_طريقو; }
    فراغ is_طريقو(bool flag) { m_is_طريقو = flag; }
    bool isLoose() ثابت { أرجع m_isLoose; }
    فراغ isLoose(bool flag) { m_isLoose = flag; }
    bool is_Proper_طريقو() ثابت { أرجع is_طريقو() && !isLoose(); }
    bool isVirtual() ثابت { أرجع m_isVirtual; }
    فراغ isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ثابت { أرجع m_needProcess; }
    فراغ سيٽ_Need_Process() { m_needProcess = true; }
    bool داخلا_نقطو() ثابت { أرجع m_داخلا_نقطو; }
    فراغ داخلا_نقطو(bool flag) { m_داخلا_نقطو = flag; }
    bool dpi_خالص() ثابت { أرجع m_dpi_خالص; }
    فراغ dpi_خالص(bool flag) { m_dpi_خالص = flag; }
    bool dpiContext() ثابت { أرجع m_dpiContext; }
    فراغ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ثابت VL_MT_محفوظ { أرجع m_dpiExportDispatcher; }
    فراغ dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ثابت { أرجع m_dpiExportImpl; }
    فراغ dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ثابت VL_MT_محفوظ { أرجع m_dpiImportPrototype; }
    فراغ dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ثابت { أرجع m_dpiImportWrapper; }
    فراغ dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ثابت { أرجع !m_dpiCDecl.خالي_جايون(); }
    ثابت string& dpiCDecl() ثابت { أرجع m_dpiCDecl; }
    فراغ dpiCDecl(ثابت string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ثابت { أرجع m_rtnType == "VlCoroutine"; }
    فراغ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ثابت { أرجع m_recursive; }
    فراغ unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ثابت override { أرجع m_unlikely; }  // Note virtual override
    فراغ noLife(bool flag) { m_noLife = flag; }
    bool noLife() ثابت { أرجع m_noLife; }
    bool isCovergroupSample() ثابت { أرجع m_isCovergroupSample; }
    فراغ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ثابت { أرجع m_voidSelf; }
    فراغ voidSelf(bool flag) { m_voidSelf = flag; }
    فراغ لاڳت(صحيح لاڳت) { m_لاڳت = لاڳت; }
    // Special methods
    bool خالي_جايون_Body() ثابت {
        أرجع !keep_If_خالي_جايون() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : عام Ast_ڳنڍ {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ثابت string m_name;
    ثابت VUseType m_useType;  // What sort of use this is

عام:
    AstCUse(File_سٽ* fl, VUseType useType, ثابت string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    VUseType useType() ثابت { أرجع m_useType; }
};
class AstCell final : عام Ast_ڳنڍ {
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
    File_سٽ* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    صحيح m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
عام:
    AstCell(File_سٽ* fl, File_سٽ* mfl, ثابت string& instName, ثابت string& modName,
            AstPin* pinsp, AstPin* paramsp, AstRange* rangep)
        : ASTGEN_SUPER_Cell(fl)
        , m_modNameFileline{mfl}
        , m_name{instName}
        , m_origName{instName}
        , m_modName{modName}
        , m_hasIfaceVar{false}
        , m_recursive{false}
        , m_trace{true} {
        جوڙيو_Pinsp(pinsp);
        جوڙيو_Paramsp(paramsp);
        جوڙيو_Rangep(rangep);
    }
    ASTGEN_MEMBERS_AstCell;
    // No cloneRelink, we presume cloneee's want the same module linkages
    فراغ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Cell name
    فراغ name(ثابت string& name) override { m_name = name; }
    string origName() ثابت override { أرجع m_origName; }  // * = Original name
    فراغ origName(ثابت string& name) { m_origName = name; }
    string modName() ثابت { أرجع m_modName; }  // * = Instance name
    فراغ modName(ثابت string& name) { m_modName = name; }
    File_سٽ* modNameFileline() ثابت { أرجع m_modNameFileline; }
    Ast_ڳنڍ_Module* modp() ثابت { أرجع m_modp; }  // [AfterLink] = Pointer to module instantiated
    فراغ modp(Ast_ڳنڍ_Module* nodep) { m_modp = nodep; }
    صحيح arrayIdx() ثابت { أرجع m_arrayIdx; }
    فراغ arrayIdx(صحيح idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ثابت { أرجع m_hasIfaceVar; }
    فراغ hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    فراغ trace(bool flag) { m_trace = flag; }
    bool isTrace() ثابت { أرجع m_trace; }
    فراغ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ثابت { أرجع m_recursive; }
};
class AstCellInline final : عام Ast_ڳنڍ {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ثابت std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ثابت std::string m_origModName;  // __BEGIN__ or __FORK__
عام:
    AstCellInline(File_سٽ* fl, ثابت std::string& name, ثابت std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // ACCESSORS
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    std::string origModName() ثابت { أرجع m_origModName; }
};
class AstCellInlineScope final : عام Ast_ڳنڍ {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ثابت std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
عام:
    AstCellInlineScope(File_سٽ* fl, ثابت std::string& name, ثابت std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // ACCESSORS
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت std::string& name) override { m_name = name; }
    std::string origModName() ثابت { أرجع m_origModName; }
};
class Ast_Cg_Option_سونپيو final : عام Ast_ڳنڍ {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ثابت VCoverOptionType m_optType;  // Option type
    ثابت string m_name;  // Original option name (for diagnostics on unknown options)
    ثابت bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
عام:
    Ast_Cg_Option_سونپيو(File_سٽ* fl, bool typeOption, VCoverOptionType optType,
                      ثابت string& rawName, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cg_Option_سونپيو(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Cg_Option_سونپيو;
    // ACCESSORS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    VCoverOptionType optType() ثابت { أرجع m_optType; }
    bool typeOption() ثابت { أرجع m_typeOption; }
};
class AstClassExtends final : عام Ast_ڳنڍ {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ثابت bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

عام:
    AstClassExtends(File_سٽ* fl, Ast_ڳنڍ* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    string verilogKwd() ثابت override { أرجع isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ثابت;
    AstClass* classp() ثابت;  // Like above, but throws error if nulll
    bool isImplements() ثابت { أرجع m_isImplements; }
    فراغ parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ثابت { أرجع m_parameterized; }
};
class AstClocking final : عام Ast_ڳنڍ {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ثابت bool m_isGlobal;  // True if global clocking

عام:
    AstClocking(File_سٽ* fl, ثابت std::string& name, Ast_Sen_وڻ* sentreep, Ast_ڳنڍ* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool isDefault() ثابت { أرجع m_isDefault; }
    bool isGlobal() ثابت { أرجع m_isGlobal; }
    AstVar* ensureEventp(bool ٻار_DType = false);
    فراغ makeDefault() { m_isDefault = true; }
};
class Ast_Clocking_شي final : عام Ast_ڳنڍ {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

عام:
    Ast_Clocking_شي(File_سٽ* fl, VDirection direction, Ast_ڳنڍ_اظهار* skewp, Ast_ڳنڍ* clockingDeclp)
        : ASTGEN_SUPER_Clocking_شي(fl) {
        m_direction = direction;
        this->skewp(skewp);
        إذا (Ast_سونپيو* ثابت clkAssignp = VN_CAST(clockingDeclp, سونپيو)) {
            this->assignp(clkAssignp);
        } وإلا {
            exprp(VN_AS(clockingDeclp, ڳنڍ_اظهار));
        }
    }
    ASTGEN_MEMBERS_Ast_Clocking_شي;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VDirection direction() ثابت { أرجع m_direction; }
    Ast_Clocking_شي* outputp() ثابت { أرجع m_outputp; }
    فراغ outputp(Ast_Clocking_شي* outputp) { m_outputp = outputp; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
};
class AstConfig final : عام Ast_ڳنڍ {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

عام:
    AstConfig(File_سٽ* fl, ثابت std::string& libname, ثابت std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_libname + "." + m_configname; }
    std::string libname() ثابت VL_MT_STABLE { أرجع m_libname; }
    std::string configname() ثابت VL_MT_STABLE { أرجع m_configname; }
};
class AstConfigCell final : عام Ast_ڳنڍ {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

عام:
    AstConfigCell(File_سٽ* fl, ثابت std::string& libname, ثابت std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_libname + "." + m_cellname; }
    std::string libname() ثابت VL_MT_STABLE { أرجع m_libname; }
    std::string cellname() ثابت VL_MT_STABLE { أرجع m_cellname; }
};
class Ast_Config_قاعدو final : عام Ast_ڳنڍ {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ثابت bool m_isCell;  // Declared as "cell" versus "instance"

عام:
    Ast_Config_قاعدو(File_سٽ* fl, Ast_ڳنڍ* cellp, Ast_ڳنڍ* usep, bool isCell)
        : ASTGEN_SUPER_Config_قاعدو(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        جوڙيو_Usep(usep);
    }
    ASTGEN_MEMBERS_Ast_Config_قاعدو;
    bool isCell() ثابت VL_MT_STABLE { أرجع m_isCell; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstConfigUse final : عام Ast_ڳنڍ {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ثابت bool m_isConfig;  // ":config"; Config, not module/primitive name

عام:
    AstConfigUse(File_سٽ* fl, ثابت std::string& libname, ثابت std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        جوڙيو_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_libname + "." + m_cellname; }
    std::string libname() ثابت VL_MT_STABLE { أرجع m_libname; }
    std::string cellname() ثابت VL_MT_STABLE { أرجع m_cellname; }
    bool isConfig() ثابت VL_MT_STABLE { أرجع m_isConfig; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstConstraint final : عام Ast_ڳنڍ {
    // Constraint
    // @astgen op1 := itemsp : List[AstNode]
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    string m_name;  // Name of constraint
    VBaseOverride m_بنياد_Override;  // BaseOverride (inital/final/extends)
    bool m_isExternDef = false;  // Extern prototype definition
    bool m_isExternExplicit = false;  // Explicit prototype declaration (has extern)
    bool m_isExternProto = false;  // Prototype declaration (implicit or explicit)
    bool m_is_Kwd_خالص = false;  // Pure constraint
    bool m_isStatic = false;  // Static constraint
عام:
    AstConstraint(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Scope name
    فراغ name(ثابت string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    فراغ بنياد_Override(ثابت VBaseOverride& flag) { m_بنياد_Override = flag; }
    VBaseOverride بنياد_Override() ثابت { أرجع m_بنياد_Override; }
    bool isExternDef() ثابت { أرجع m_isExternDef; }
    فراغ isExternDef(bool flag) { m_isExternDef = flag; }
    فراغ isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ثابت { أرجع m_isExternExplicit; }
    فراغ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ثابت { أرجع m_isExternProto; }
    فراغ is_Kwd_خالص(bool flag) { m_is_Kwd_خالص = flag; }
    bool is_Kwd_خالص() ثابت { أرجع m_is_Kwd_خالص; }
    فراغ isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ثابت { أرجع m_isStatic; }
};
class AstCoverBin final : عام Ast_ڳنڍ {
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

عام:
    AstCoverBin(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_ڳنڍ_اظهار* iffp = nullptr)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{isIllegal ? VCoverBinsType::BINS_ILLEGAL
                               : (isIgnore ? VCoverBinsType::BINS_IGNORE
                                           : (isWildcard ? VCoverBinsType::BINS_WILDCARD
                                                         : VCoverBinsType::BINS_USER))}
        , m_isWildcard{isWildcard} {
        جوڙيو_Rangesp(rangesp);
        this->iffp(iffp);
    }
    // Constructor for automatic bins
    AstCoverBin(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_اظهار* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_سٽ* fl, ثابت string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_سٽ* fl, ثابت string& name, Ast_Cover_Trans_سيٽ* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        جوڙيو_Transp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    VCoverBinsType binsType() ثابت { أرجع m_binsType; }
    bool isWildcard() ثابت { أرجع m_isWildcard; }
    bool isArray() ثابت { أرجع m_isArray; }
    فراغ isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : عام Ast_ڳنڍ {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ثابت bool m_isNegated;  // Complement the selection within the cross product

عام:
    AstCoverBinsof(File_سٽ* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_ڳنڍ* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        جوڙيو_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    bool isNegated() ثابت { أرجع m_isNegated; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverBinsof* ثابت asamep = VN_DBG_AS(samep, CoverBinsof);
        أرجع m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : عام Ast_ڳنڍ {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ثابت string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ثابت VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

عام:
    AstCoverCrossBin(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* selectp, Ast_ڳنڍ_اظهار* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    string verilogKwd() ثابت override;
    VCoverBinsType binsType() ثابت { أرجع m_binsType; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverCrossBin* ثابت asamep = VN_DBG_AS(samep, CoverCrossBin);
        أرجع m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : عام Ast_ڳنڍ {
    // The complete bin-tuple space of the enclosing cross
    ثابت string m_name;  // Referenced cross name

عام:
    AstCoverCrossRef(File_سٽ* fl, ثابت string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {  // LCOV_EXCL_START
        أرجع m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : عام Ast_ڳنڍ {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ثابت bool m_isOr;  // Union (||), rather than intersection (&&)

عام:
    AstCoverCrossSelect(File_سٽ* fl, Ast_ڳنڍ* lhsp, Ast_ڳنڍ* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isOr() ثابت { أرجع m_isOr; }
    string verilogKwd() ثابت override { أرجع isOr() ? "||" : "&&"; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {  // LCOV_EXCL_START
        أرجع m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : عام Ast_ڳنڍ {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
عام:
    AstCoverCrossWith(File_سٽ* fl, Ast_ڳنڍ* selectp, Ast_ڳنڍ_اظهار* filterp, Ast_ڳنڍ_اظهار* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : عام Ast_ڳنڍ {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ثابت VCoverOptionType m_optType;  // Option being assigned
    ثابت bool m_typeOption;  // type_option (vs option)

عام:
    AstCoverOption(File_سٽ* fl, bool typeOption, VCoverOptionType optType, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VCoverOptionType optType() ثابت { أرجع m_optType; }
    bool typeOption() ثابت { أرجع m_typeOption; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverOption* ثابت asamep = VN_DBG_AS(samep, CoverOption);
        أرجع m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : عام Ast_ڳنڍ {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
عام:
    AstCoverToggle(File_سٽ* fl, AstCoverInc* incp, Ast_ڳنڍ_اظهار* origp, Ast_ڳنڍ_اظهار* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    صحيح instr_ڳڻپ() ثابت override { أرجع 3 + INSTR_ڳڻپ_BRANCH + INSTR_ڳڻپ_LD; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool isOutputter() override {
        أرجع false;  // Though the AstCoverInc under this is an outputter
    }
};
class Ast_Cover_Trans_شي final : عام Ast_ڳنڍ {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ثابت VTransRepType m_repType;

عام:
    Ast_Cover_Trans_شي(File_سٽ* fl, Ast_ڳنڍ* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_Cover_Trans_شي(fl)
        , m_repType{repType} {
        جوڙيو_Valuesp(valuesp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_شي;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class Ast_Cover_Trans_سيٽ final : عام Ast_ڳنڍ {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
عام:
    Ast_Cover_Trans_سيٽ(File_سٽ* fl, Ast_Cover_Trans_شي* itemsp)
        : ASTGEN_SUPER_Cover_Trans_سيٽ(fl) {
        جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_سيٽ;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstCoverWith final : عام Ast_ڳنڍ {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
عام:
    AstCoverWith(File_سٽ* fl, Ast_ڳنڍ* subp, Ast_ڳنڍ_اظهار* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        جوڙيو_Subp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : عام Ast_ڳنڍ {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

عام:
    AstCovergroup(File_سٽ* fl, ثابت string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_ڳنڍ* membersp, Ast_Sen_وڻ* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        جوڙيو_Argsp(argsp);
        جوڙيو_Sample_Argsp(sampleArgsp);
        جوڙيو_Membersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    bool maybe_Pointed_منزل() ثابت override { أرجع true; }
};
class AstCoverpointRef final : عام Ast_ڳنڍ {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ثابت string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

عام:
    AstCoverpointRef(File_سٽ* fl, ثابت string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstDefParam final : عام Ast_ڳنڍ {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
عام:
    AstDefParam(File_سٽ* fl, Ast_ڳنڍ_اظهار* pathp, ثابت string& name, Ast_ڳنڍ_اظهار* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع true; }
};
class AstDefaultClocking final : عام Ast_ڳنڍ {
    std::string m_name;  // Clocking block name

عام:
    AstDefaultClocking(File_سٽ* fl, ثابت std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstDefaultDisable final : عام Ast_ڳنڍ {
    // @astgen op1 := condp : AstNodeExpr

عام:
    AstDefaultDisable(File_سٽ* fl, Ast_ڳنڍ_اظهار* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : عام Ast_ڳنڍ {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
عام:
    AstDpiExport(File_سٽ* fl, ثابت string& vname, ثابت string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    string cname() ثابت { أرجع m_cname; }
    فراغ cname(ثابت string& cname) { m_cname = cname; }
};
class AstElabDisplay final : عام Ast_ڳنڍ {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

عام:
    مضمن AstElabDisplay(File_سٽ* fl, VDisplayType dispType, Ast_ڳنڍ_اظهار* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!fmtp());
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string verilogKwd() ثابت override { أرجع "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { أرجع true; }  // SPECIAL: $display makes output
    bool isUnlikely() ثابت override { أرجع true; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    صحيح instr_ڳڻپ() ثابت override { أرجع INSTR_ڳڻپ_PLI; }
    VDisplayType displayType() ثابت { أرجع m_displayType; }
    فراغ displayType(VDisplayType type) { m_displayType = type; }
};
class Ast_خالي_جايون final : عام Ast_ڳنڍ {
    // Represents something missing, e.g. a missing argument in FOREACH
عام:
    explicit Ast_خالي_جايون(File_سٽ* fl)
        : ASTGEN_SUPER_خالي_جايون(fl) {}
    ASTGEN_MEMBERS_Ast_خالي_جايون;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class Ast_Gen_Case_شي final : عام Ast_ڳنڍ {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
عام:
    Ast_Gen_Case_شي(File_سٽ* fl, Ast_ڳنڍ_اظهار* condsp, Ast_ڳنڍ* itemsp)
        : ASTGEN_SUPER_Gen_Case_شي(fl) {
        جوڙيو_Condsp(condsp);
        جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_Case_شي;
    bool isDefault() ثابت { أرجع condsp() == nullptr; }
};
class AstImplicit final : عام Ast_ڳنڍ {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
عام:
    AstImplicit(File_سٽ* fl, Ast_ڳنڍ* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        جوڙيو_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : عام Ast_ڳنڍ {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_والد_Name;  // Hierarchical path of the declaring module, before inlining
    string m_بنياد_Name;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
عام:
    AstIntfRef(File_سٽ* fl, ثابت string& name, ثابت string& والد_Name, ثابت string& بنياد_Name,
               ثابت string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_والد_Name{والد_Name}
        , m_بنياد_Name{بنياد_Name}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    string والد_Name() ثابت { أرجع m_والد_Name; }
    string بنياد_Name() ثابت { أرجع m_بنياد_Name; }
    string modportName() ثابت { أرجع m_modportName; }
};
class AstLibrary final : عام Ast_ڳنڍ {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

عام:
    AstLibrary(File_سٽ* fl, ثابت std::string& name, Ast_ڳنڍ_اظهار* filesp, Ast_ڳنڍ_اظهار* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        جوڙيو_Filesp(filesp);
        جوڙيو_Incdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstModport final : عام Ast_ڳنڍ {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
عام:
    AstModport(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        جوڙيو_Varsp(varsp);
    }
    string verilogKwd() ثابت override { أرجع "modport"; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : عام Ast_ڳنڍ {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
عام:
    AstModportClockingRef(File_سٽ* fl, ثابت string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    فراغ dump(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    AstClocking* clockingp() ثابت VL_MT_STABLE {
        أرجع m_clockingp;
    }  // [After Link] Pointer to clocking block
    فراغ clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : عام Ast_ڳنڍ {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
عام:
    AstModportFTaskRef(File_سٽ* fl, ثابت string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool isImport() ثابت { أرجع !m_export; }
    bool isExport() ثابت { أرجع m_export; }
    Ast_ڳنڍ_FTask* ftaskp() ثابت { أرجع m_ftaskp; }  // [After Link] Pointer to variable
    فراغ ftaskp(Ast_ڳنڍ_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : عام Ast_ڳنڍ {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ثابت VDirection m_direction;  // Direction of the variable (in/out)
عام:
    AstModportVarRef(File_سٽ* fl, ثابت string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_اظهار* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    VDirection direction() ثابت { أرجع m_direction; }
    AstVar* varp() ثابت VL_MT_STABLE { أرجع m_varp; }  // [After Link] Pointer to variable
    فراغ varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : عام Ast_ڳنڍ {
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
    std::string m_resolved_چوٽي_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::سيٽ<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ثابت AstConst*, string> m_const_Orig_Param_نالا;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ثابت Ast_ڳنڍ*, ثابت Ast_ڳنڍ_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::ENUM_پڇاڙي> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::ENUM_پڇاڙي> m_dumpTriggersFuncps{};

عام:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ثابت حرف* broken() ثابت override;
    فراغ ڌڪو_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.وجهو(varp); }
    ثابت std::سيٽ<AstVar*>& deferredParamVarps() ثابت { أرجع m_deferredParamVarps; }
    فراغ clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    فراغ deleteContents();
    فراغ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ثابت override VL_MT_STABLE { أرجع "$root"; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    Ast_ڳنڍ_Module* چوٽي_Modulep() ثابت VL_MT_STABLE {  // Top module in hierarchy
        أرجع modulesp();  // First one in the list, for now
    }
    Ast_Type_جدول* typeTablep() { أرجع m_typeTablep; }
    string astConstOrigParamName(ثابت AstConst* nodep) ثابت;
    فراغ astConstOrigParamName(ثابت AstConst* nodep, ثابت string& name);
    فراغ astConstOrigParamNameErase(ثابت AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ثابت Ast_ڳنڍ_Module* containingModule(ثابت Ast_ڳنڍ* nodep);
    // Forget remembered modules, as the tree has moved.
    فراغ clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ثابت { أرجع m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ثابت { أرجع m_dollarUnitPkgp; }
    فراغ dollarUnitPkgp(AstPackage* ثابت packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ثابت { أرجع m_dpiExportTriggerp; }
    فراغ dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ثابت { أرجع m_delaySchedulerp; }
    فراغ delaySchedulerp(AstVar* ثابت varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ثابت { أرجع m_nbaEventp; }
    فراغ nbaEventp(AstVarScope* ثابت varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ثابت { أرجع m_nbaEventTriggerp; }
    فراغ nbaEventTriggerp(AstVarScope* ثابت varScopep) { m_nbaEventTriggerp = varScopep; }
    فراغ stdPackagep(AstPackage* ثابت packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ثابت { أرجع m_stdPackagep; }
    فراغ stdPackageProcessp(AstClass* ثابت classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ثابت { أرجع m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_سٽ*) ثابت;
    Ast_چوٽي_Scope* چوٽي_Scopep() ثابت { أرجع m_چوٽي_Scopep; }
    فراغ create_چوٽي_Scope(AstScope* scopep);
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& قدر) { m_timeunit = قدر; }
    VTimescale timeprecision() ثابت { أرجع m_timeprecision; }
    فراغ وقت_Init() {
        m_timeunit = v3Global.opt.وقت_Default_Unit();
        m_timeprecision = v3Global.opt.وقت_Default_Prec();
    }
    فراغ timeprecision_ملايو(File_سٽ*, ثابت VTimescale& قدر);
    فراغ timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ثابت { أرجع m_timescaleSpecified; }
    ثابت std::string& resolved_چوٽي_Module_Name() ثابت { أرجع m_resolved_چوٽي_Module_Name; }
    فراغ resolved_چوٽي_Module_Name(ثابت std::string& قدر) { m_resolved_چوٽي_Module_Name = قدر; }
    uint32_t nTraceCodes() ثابت { أرجع m_nTraceCodes; }
    فراغ nTraceCodes(uint32_t قدر) { m_nTraceCodes = قدر; }
    AstVarScope* stl_پهريون_Iterationp();
    فراغ clear_Stl_پهريون_Iterationp() { m_stl_پهريون_Iterationp = nullptr; }
    ثابت std::string trace_Lib_چوٽي_Name() ثابت {
        ثابت std::string& name = resolved_چوٽي_Module_Name();
        أرجع prettyName(name.خالي_جايون() ? v3Global.rootp()->چوٽي_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    فراغ جوڙيو_Eval_Stats(ثابت std::string& phase);
};
class AstPackageExport final : عام Ast_ڳنڍ {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

عام:
    AstPackageExport(File_سٽ* fl, ثابت string& pkgName, ثابت string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    string pkgName() ثابت VL_MT_STABLE { أرجع m_pkgName; }
    string prettyPkgNameQ() ثابت { أرجع "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ثابت { أرجع m_packagep; }
    فراغ packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class Ast_Package_Export_تارو_تارو final : عام Ast_ڳنڍ {
    // A package export *::* declaration
عام:
    // cppcheck-suppress noExplicitConstructor
    Ast_Package_Export_تارو_تارو(File_سٽ* fl)
        : ASTGEN_SUPER_Package_Export_تارو_تارو(fl) {}
    ASTGEN_MEMBERS_Ast_Package_Export_تارو_تارو;
};
class AstPackageImport final : عام Ast_ڳنڍ {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

عام:
    AstPackageImport(File_سٽ* fl, AstPackage* packagep, ثابت string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_سٽ* fl, ثابت string& pkgName, ثابت string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    string pkgName() ثابت VL_MT_STABLE { أرجع m_pkgName; }
    string prettyPkgNameQ() ثابت { أرجع "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ثابت { أرجع m_packagep; }
    فراغ packagep(AstPackage* nodep) { m_packagep = nodep; }

خاص:
    فراغ pkgNameFrom();
};
class AstPin final : عام Ast_ڳنڍ {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    صحيح m_pin_انگ;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_param_رستو;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
عام:
    AstPin(File_سٽ* fl, صحيح pin_انگ, ثابت string& name, Ast_ڳنڍ* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_انگ{pin_انگ}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    فراغ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Pin name, ""=go by number
    فراغ name(ثابت string& name) override { m_name = name; }
    string prettyOperatorName() ثابت override;
    bool dot_تارو() ثابت { أرجع name() == ".*"; }  // Fake name for .* connections until linked
    صحيح pin_انگ() ثابت { أرجع m_pin_انگ; }
    AstVar* modVarp() ثابت { أرجع m_modVarp; }  // [After Link] Pointer to variable
    فراغ modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ثابت { أرجع m_modPTypep; }
    فراغ modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ثابت { أرجع m_param; }
    فراغ param(bool flag) { m_param = flag; }
    ثابت string& param_رستو() ثابت { أرجع m_param_رستو; }
    فراغ param_رستو(ثابت string& رستو) { m_param_رستو = رستو; }
    bool svDotName() ثابت { أرجع m_svDotName; }
    فراغ svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ثابت { أرجع m_svImplicit; }
    فراغ svImplicit(bool flag) { m_svImplicit = flag; }
};
class Ast_پورٽ final : عام Ast_ڳنڍ {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ثابت صحيح m_pin_انگ;  // Pin number
    ثابت string m_name;  // Name of pin
عام:
    Ast_پورٽ(File_سٽ* fl, صحيح pinnum, ثابت string& name)
        : ASTGEN_SUPER_پورٽ(fl)
        , m_pin_انگ{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_پورٽ;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Port name
    صحيح pin_انگ() ثابت { أرجع m_pin_انگ; }  // * = Pin number, for order based instantiation
};
class AstPragma final : عام Ast_ڳنڍ {
    ثابت VPragmaType m_pragType;  // Type of pragma
    ثابت VTimescale m_timescale;  // For TIMEUNIT_SET
عام:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_سٽ* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_سيٽ, fl, "Should use other constructor");
    }
    AstPragma(File_سٽ* fl, ثابت VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_سيٽ}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VPragmaType pragType() ثابت { أرجع m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ثابت { أرجع m_timescale; }
};
class AstPropSpec final : عام Ast_ڳنڍ {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

عام:
    AstPropSpec(File_سٽ* fl, Ast_Sen_شي* sensesp, Ast_ڳنڍ_اظهار* disablep, Ast_ڳنڍ* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ {
        أرجع true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ثابت { أرجع m_propStrength; }
};
class AstPull final : عام Ast_ڳنڍ {
    // @astgen op1 := lhsp : AstNodeExpr

    ثابت bool m_direction;

عام:
    AstPull(File_سٽ* fl, Ast_ڳنڍ_اظهار* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ثابت { أرجع m_direction; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstScope final : عام Ast_ڳنڍ {
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
عام:
    AstScope(File_سٽ* fl, Ast_ڳنڍ_Module* modp, ثابت string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!m_modp);
        أرجع nullptr;
    }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Scope name
    فراغ name(ثابت string& name) override { m_name = name; }
    string pretty_Name_نياپو() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override;
    string nameDotless() ثابت;
    Ast_ڳنڍ_Module* modp() ثابت { أرجع m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    Ast_سرگرم* comboActivep(bool create);
    //
    AstScope* aboveScopep() ثابت VL_MT_محفوظ { أرجع m_aboveScopep; }
    فراغ aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ثابت { أرجع m_aboveCellp; }
    فراغ aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_چوٽي() ثابت VL_MT_محفوظ { أرجع aboveScopep() == nullptr; }  // At top of hierarchy
};
class Ast_Sen_شي final : عام Ast_ڳنڍ {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_ڪنارو_Type;  // Edge type
عام:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    Ast_Sen_شي(File_سٽ* fl, VEdgeType ڪنارو_Type, Ast_ڳنڍ_اظهار* senp, Ast_ڳنڍ_اظهار* condp = nullptr)
        : ASTGEN_SUPER_Sen_شي(fl)
        , m_ڪنارو_Type{ڪنارو_Type} {
        this->sensp(senp);
        this->condp(condp);
    }
    Ast_Sen_شي(File_سٽ* fl, Combo)
        : ASTGEN_SUPER_Sen_شي(fl)
        , m_ڪنارو_Type{VEdgeType::ET_COMBO} {}
    Ast_Sen_شي(File_سٽ* fl, Static)
        : ASTGEN_SUPER_Sen_شي(fl)
        , m_ڪنارو_Type{VEdgeType::ET_STATIC} {}
    Ast_Sen_شي(File_سٽ* fl, Initial)
        : ASTGEN_SUPER_Sen_شي(fl)
        , m_ڪنارو_Type{VEdgeType::ET_INITIAL} {}
    Ast_Sen_شي(File_سٽ* fl, InitialNBA)
        : ASTGEN_SUPER_Sen_شي(fl)
        , m_ڪنارو_Type{VEdgeType::ET_INITIAL_NBA} {}
    Ast_Sen_شي(File_سٽ* fl, Final)
        : ASTGEN_SUPER_Sen_شي(fl)
        , m_ڪنارو_Type{VEdgeType::ET_FINAL} {}
    Ast_Sen_شي(File_سٽ* fl, Never)
        : ASTGEN_SUPER_Sen_شي(fl)
        , m_ڪنارو_Type{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_Ast_Sen_شي;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع ڪنارو_Type() == VN_DBG_AS(samep, Sen_شي)->ڪنارو_Type();
    }
    VEdgeType ڪنارو_Type() ثابت { أرجع m_ڪنارو_Type; }
    فراغ ڪنارو_Type(VEdgeType type) {
        m_ڪنارو_Type = type;
        edit_ڳڻپ_Inc();
    }
    Ast_ڳنڍ_Var_Ref* varrefp() ثابت { أرجع VN_CAST(sensp(), ڳنڍ_Var_Ref); }
    //
    bool isClocked() ثابت { أرجع ڪنارو_Type().clockedStmt(); }
    bool is_Combo_Or_تارو() ثابت {
        أرجع ڪنارو_Type() == VEdgeType::ET_COMBO || ڪنارو_Type() == VEdgeType::ET_COMBO_تارو;
    }
    bool is_Combo_تارو() ثابت { أرجع ڪنارو_Type() == VEdgeType::ET_COMBO_تارو; }
    bool isHybrid() ثابت { أرجع ڪنارو_Type() == VEdgeType::ET_HYBRID; }
    bool isStatic() ثابت { أرجع ڪنارو_Type() == VEdgeType::ET_STATIC; }
    bool isInitial() ثابت { أرجع ڪنارو_Type() == VEdgeType::ET_INITIAL; }
    bool isFinal() ثابت { أرجع ڪنارو_Type() == VEdgeType::ET_FINAL; }
    bool isNever() ثابت { أرجع ڪنارو_Type() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_وڻ final : عام Ast_ڳنڍ {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
عام:
    Ast_Sen_وڻ(File_سٽ* fl, Ast_Sen_شي* sensesp)
        : ASTGEN_SUPER_Sen_وڻ(fl) {
        جوڙيو_Sensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_وڻ;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    // METHODS
    bool hasClocked() ثابت;  // Includes a clocked statement
    bool has_ڪنارو() ثابت;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ثابت;  // Includes a STATIC SenItem
    bool hasInitial() ثابت;  // Includes a INITIAL SenItem
    bool hasFinal() ثابت;  // Includes a FINAL SenItem
    bool hasCombo() ثابت;  // Includes a COMBO SenItem
    bool hasHybrid() ثابت;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : عام Ast_ڳنڍ {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

عام:
    AstStrengthSpec(File_سٽ* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { أرجع m_s0; }
    VStrength strength1() { أرجع m_s1; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstSystemCSection final : عام Ast_ڳنڍ {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ثابت VSystemCSectionType m_sectionType;  // The section type
    std::string m_متن;  // The text content

عام:
    AstSystemCSection(File_سٽ* fl, VSystemCSectionType sectionType, ثابت std::string& متن)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_متن{متن} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ثابت { أرجع m_sectionType; }
    ثابت std::string& متن() ثابت { أرجع m_متن; }
    فراغ متن(ثابت std::string& قدر) { m_متن = قدر; }
    فراغ dump(std::ostream&) ثابت override;
    فراغ dumpJson(std::ostream&) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ*) ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
};
class Ast_متن final : عام Ast_ڳنڍ {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ثابت std::string m_متن;  // The text to emit
عام:
    Ast_متن(File_سٽ* fl, ثابت std::string& متن)
        : ASTGEN_SUPER_متن(fl)
        , m_متن{متن} {}
    ASTGEN_MEMBERS_Ast_متن;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع متن() == VN_DBG_AS(samep, متن_2)->متن();
    }
    ثابت std::string& متن() ثابت VL_MT_محفوظ { أرجع m_متن; }
};
class Ast_متن_بلاڪ final : عام Ast_ڳنڍ {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_اڳياڙي;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
عام:
    explicit Ast_متن_بلاڪ(File_سٽ* fl,  //
                          ثابت std::string& اڳياڙي = "",  //
                          ثابت std::string& separator = "",  //
                          ثابت std::string& suffix = "")
        : ASTGEN_SUPER_متن_بلاڪ(fl)
        , m_اڳياڙي{اڳياڙي}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_Ast_متن_بلاڪ;
    ثابت std::string& اڳياڙي() ثابت { أرجع m_اڳياڙي; }
    ثابت std::string& separator() ثابت { أرجع m_separator; }
    ثابت std::string& suffix() ثابت { أرجع m_suffix; }
    // Add some text, or a node to this block
    فراغ جوڙيو(ثابت string& متن) { جوڙيو_Nodesp(new Ast_متن{fileline(), متن}); }
    فراغ جوڙيو(Ast_ڳنڍ* nodep) { جوڙيو_Nodesp(nodep); }
};
class Ast_چوٽي_Scope final : عام Ast_ڳنڍ {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    دوست class AstNetlist;  // Only the AstNetlist can create one
    Ast_چوٽي_Scope(File_سٽ* fl, AstScope* ascopep)
        : ASTGEN_SUPER_چوٽي_Scope(fl) {
        this->scopep(ascopep);
    }

عام:
    ASTGEN_MEMBERS_Ast_چوٽي_Scope;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_عارضي(ثابت string& name, بنانشان ويڪر);
    AstVarScope* create_عارضي(ثابت string& name, Ast_ڳنڍ_DType* dtypep);
};
class Ast_Type_جدول final : عام Ast_ڳنڍ {
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
    using DetailedMap = std::map<VBasic_Type_ڪنجي, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

عام:
    explicit Ast_Type_جدول(File_سٽ* fl);
    ASTGEN_MEMBERS_Ast_Type_جدول;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    فراغ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* ڳوليو_Basic_DType(File_سٽ* fl, VBasicDTypeKwd kwd);
    AstBasicDType* ڳوليو_Logic_ٻٽو_انگ_DType(File_سٽ* fl, VBasicDTypeKwd kwd, صحيح ويڪر, صحيح ويڪر_Min,
                                     VSigning numeric);
    AstBasicDType* ڳوليو_Logic_ٻٽو_انگ_DType(File_سٽ* fl, VBasicDTypeKwd kwd, ثابت VNumRange& range,
                                     صحيح ويڪر_Min, VSigning numeric);
    AstBasicDType* ڳوليو_Create_Same_DType(AstBasicDType& ڳنڍ);
    AstBasicDType* ڳوليو_وجهو_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* ڳوليو_Constraint_Ref_DType(File_سٽ* fl);
    Ast_خالي_جايون_قطار_DType* ڳوليو_خالي_جايون_قطار_DType(File_سٽ* fl);
    Ast_قطار_DType* ڳوليو_قطار_اشاريو_DType(File_سٽ* fl);
    Ast_وهڪرو_DType* ڳوليو_وهڪرو_DType(File_سٽ* fl);
    AstVoidDType* ڳوليو_Void_DType(File_سٽ* fl);
    فراغ clearCache();
    فراغ repairCache();
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstTypedef final : عام Ast_ڳنڍ {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_ٽيگ;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_انگ;  // Declaration token number
    bool m_attr_عوامي = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

عام:
    AstTypedef(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* attrsp, VFlag_ٻار_DType,
               Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_انگ{fl->token_انگ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        ٻار_DTypep(dtp);  // Only for parser
        جوڙيو_Attrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_انگ{fl->token_انگ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ثابت;
    Ast_ڳنڍ_DType* get_ٻار_DTypep() ثابت override { أرجع ٻار_DTypep(); }
    مجازي Ast_ڳنڍ_DType* subDTypep() ثابت VL_MT_STABLE {
        أرجع dtypep() ? dtypep() : ٻار_DTypep();
    }
    // METHODS
    uint32_t decl_Token_انگ() ثابت override { أرجع m_decl_Token_انگ; }
    فراغ decl_Token_انگ_سيٽ_Min(uint32_t token_انگ) override {
        m_decl_Token_انگ = std::min(m_decl_Token_انگ, token_انگ);
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    فراغ name(ثابت string& flag) override { m_name = flag; }
    bool attr_عوامي() ثابت { أرجع m_attr_عوامي; }
    فراغ attr_عوامي(bool flag) { m_attr_عوامي = flag; }
    bool isHideLocal() ثابت { أرجع m_isHideLocal; }
    فراغ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ثابت { أرجع m_isHideProtected; }
    فراغ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ثابت { أرجع m_isUnderClass; }
    فراغ isUnderClass(bool flag) { m_isUnderClass = flag; }
    فراغ ٽيگ(ثابت string& متن) override { m_ٽيگ = متن; }
    string ٽيگ() ثابت override { أرجع m_ٽيگ; }
};
class AstTypedefFwd final : عام Ast_ڳنڍ {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ثابت VFwdType m_fwdType;  // Forward type for lint check

عام:
    AstTypedefFwd(File_سٽ* fl, ثابت string& name, ثابت VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    VFwdType fwdType() ثابت { أرجع m_fwdType; }
};
class Ast_Udp_جدول final : عام Ast_ڳنڍ {
    // @astgen op1 := linesp : List[AstUdpTableLine]
عام:
    Ast_Udp_جدول(File_سٽ* fl, Ast_Udp_جدول_سٽ* linesp)
        : ASTGEN_SUPER_Udp_جدول(fl) {
        this->جوڙيو_Linesp(linesp);
        إذا (!v3Global.has_جدول()) v3Global.سيٽ_Has_جدول();
    }
    ASTGEN_MEMBERS_Ast_Udp_جدول;
};
class Ast_Udp_جدول_سٽ final : عام Ast_ڳنڍ {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
خاص:
    ثابت bool m_udpIsCombo;  // Combinational or sequential UDP

عام:
    class UdpCombo {};
    Ast_Udp_جدول_سٽ(UdpCombo, File_سٽ* fl, Ast_Udp_جدول_سٽ_Val* iFieldsp,
                    Ast_Udp_جدول_سٽ_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_جدول_سٽ(fl)
        , m_udpIsCombo{true} {
        جوڙيو_IFieldsp(iFieldsp);
        جوڙيو_OFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_جدول_سٽ(UdpSequential, File_سٽ* fl, Ast_Udp_جدول_سٽ_Val* iFieldsp,
                    Ast_Udp_جدول_سٽ_Val* oFieldsp1, Ast_Udp_جدول_سٽ_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_جدول_سٽ(fl)
        , m_udpIsCombo{false} {
        جوڙيو_IFieldsp(iFieldsp);
        جوڙيو_OFieldsp(oFieldsp1);
        جوڙيو_OFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_جدول_سٽ;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح udpIsCombo() ثابت { أرجع m_udpIsCombo; }
};
class Ast_Udp_جدول_سٽ_Val final : عام Ast_ڳنڍ {
    string m_متن;  // Value character

عام:
    Ast_Udp_جدول_سٽ_Val(File_سٽ* fl, ثابت string& متن)
        : ASTGEN_SUPER_Udp_جدول_سٽ_Val(fl)
        , m_متن{متن} {}
    ASTGEN_MEMBERS_Ast_Udp_جدول_سٽ_Val;
    string name() ثابت override VL_MT_STABLE { أرجع m_متن; }
    فراغ name(std::string ثابت& متن) override VL_MT_STABLE { m_متن = متن; }
    string متن() ثابت VL_MT_محفوظ { أرجع m_متن; }
};
class AstVar final : عام Ast_ڳنڍ {
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
    string m_ٽيگ;  // Holds the string of the verilator tag -- used in JSON output.
    VVarType m_varType;  // Type of variable
    VDirection m_direction;  // Direction input/output etc
    VDirection m_declDirection;  // Declared direction input/output etc
    VLifetime m_lifetime;  // Lifetime
    VRandAttr m_rand;  // Randomizability of this variable (rand, randc, etc)
    صحيح m_pin_انگ = 0;  // For JSON, if non-zero the connection pin number
    صحيح m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
    bool m_libInoutEnable : 1;  // Port is the lowered library inout's enable
    bool m_ansi : 1;  // Params or pins declared in the module header, rather than the body
    bool m_declTyped : 1;  // Declared as type (for dedup check)
    bool m_tristate : 1;  // Inout or triwire or trireg
    bool m_primaryIO : 1;  // In/out to top level (or directly assigned from same)
    bool m_primaryClock : 1;  // In/out to top level and is, or combinationally drives a SenItem
    bool m_sc : 1;  // SystemC variable
    bool m_scSensitive : 1;  // SystemC sensitive() needed
    bool m_sig_عوامي : 1;  // User C code accesses this signal or is top signal
    bool m_sig_Mod_عوامي : 1;  // User C code accesses this signal and module
    bool m_sig_User_Rd_عوامي : 1;  // User C code accesses this signal, read only
    bool m_sigUserRWPublic : 1;  // User C code accesses this signal, read-write
    bool m_استعمال_ٿيل_Param : 1;  // Parameter is referenced (on link; later signals not setup)
    bool m_استعمال_ٿيل_Loop_Idx : 1;  // Variable subject of for unrolling
    bool m_funcLocal : 1;  // Local variable for a function
    bool m_funcLocalSticky : 1;  // As m_funcLocal but remains set if var is moved to a static
    bool m_funcReturn : 1;  // Return variable for a function
    bool m_attrScBv : 1;  // User force bit vector attribute
    bool m_attrScBigUint : 1;  // User force sc_biguint attribute
    bool m_attrSFormat : 1;  // User sformat attribute
    bool m_attr_ورهايو_Var : 1;  // declared with split_var metacomment
    bool m_attrFsmState : 1;  // declared with fsm_state metacomment
    bool m_attrFsmArcInclCond : 1;  // declared with fsm_arc_include_cond metacomment
    VFsmExpandType::en m_attrFsmStateExpand : 2;  // FSM expansion type for FSM coverage
    bool m_const_Pool_داخلا : 1;  // Constant pool variable
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
    bool m_is_Iface_Array_ورهايو : 1;  // Interface array that was split into element variables by
                                   // V3Param, until V3Width removes it
    bool m_is_اندروني : 1;  // Internal state, don't add to method pinter
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
    bool m_is_Forced_By_ڪوڊ : 1;  // May be forced/released from AstAssignForce/AstRelease
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
    bool m_mtask_Cache_سٽ_Align : 1;  // Start MTask affinity group on a cache line
    فراغ init() {
        m_libInoutEnable = false;
        m_ansi = false;
        m_declTyped = false;
        m_tristate = false;
        m_primaryIO = false;
        m_primaryClock = false;
        m_sc = false;
        m_scSensitive = false;
        m_استعمال_ٿيل_Param = false;
        m_استعمال_ٿيل_Loop_Idx = false;
        m_sig_عوامي = false;
        m_sig_Mod_عوامي = false;
        m_sig_User_Rd_عوامي = false;
        m_sigUserRWPublic = false;
        m_funcLocal = false;
        m_funcLocalSticky = false;
        m_funcReturn = false;
        m_attrScBv = false;
        m_attrScBigUint = false;
        m_attrSFormat = false;
        m_attr_ورهايو_Var = false;
        m_attrFsmState = false;
        m_attrFsmArcInclCond = false;
        m_attrFsmStateExpand = VFsmExpandType::DEFAULT;
        m_const_Pool_داخلا = false;
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
        m_is_Iface_Array_ورهايو = false;
        m_is_اندروني = false;
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
        m_is_Forced_By_ڪوڊ = false;
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
        m_mtask_Cache_سٽ_Align = false;
    }

عام:
    AstVar(File_سٽ* fl, VVarType type, ثابت string& name, VFlag_ٻار_DType, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ٻار_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_سٽ* fl, VVarType type, ثابت string& name, Ast_ڳنڍ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_سٽ* fl, VVarType type, ثابت string& name, VFlagLogicPacked, صحيح wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_سيٽ_Logic_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_سٽ* fl, VVarType type, ثابت string& name, VFlag_ٻٽو_انگ_Packed, صحيح wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_سيٽ_ٻٽو_انگ_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_سٽ* fl, VVarType type, ثابت string& name, ثابت AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        إذا (examplep->ٻار_DTypep()) ٻار_DTypep(examplep->ٻار_DTypep()->clone_وڻ(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    مضمن bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    string origName() ثابت override { أرجع m_origName; }  // * = Original name
    فراغ origName(ثابت string& name) { m_origName = name; }
    VVarType varType() ثابت VL_MT_محفوظ { أرجع m_varType; }  // * = Type of variable
    فراغ direction(ثابت VDirection& flag) {
        m_direction = flag;
        إذا (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ثابت VL_MT_محفوظ { أرجع m_direction; }
    bool isIO() ثابت VL_MT_محفوظ { أرجع m_direction != VDirection::NONE; }
    bool isVLIO() ثابت {
        ثابت AstBasicDType* ثابت bdtypep = basicp();
        أرجع isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    فراغ declDirection(ثابت VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ثابت { أرجع m_declDirection; }
    صحيح libInoutId() ثابت { أرجع m_libInoutId; }
    فراغ libInoutId(صحيح id) { m_libInoutId = id; }
    bool libInoutEnable() ثابت { أرجع m_libInoutEnable; }
    فراغ libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    فراغ varType(VVarType type) { m_varType = type; }
    فراغ var_Type2_ٻاهريون() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    فراغ var_Type2_اندريون() {
        m_tristate = false;
        m_direction = VDirection::داخلا;
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
    فراغ combineType(VVarType type);
    Ast_ڳنڍ_DType* get_ٻار_DTypep() ثابت override { أرجع ٻار_DTypep(); }
    Ast_ڳنڍ_DType* dtypeSkipRefp() ثابت VL_MT_STABLE { أرجع subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ثابت VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    مجازي Ast_ڳنڍ_DType* subDTypep() ثابت VL_MT_STABLE {
        أرجع dtypep() ? dtypep() : ٻار_DTypep();
    }
    فراغ ansi(bool flag) { m_ansi = flag; }
    فراغ declTyped(bool flag) { m_declTyped = flag; }
    فراغ sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    فراغ attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    فراغ attrScBv(bool flag) { m_attrScBv = flag; }
    فراغ attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    فراغ attrSFormat(bool flag) { m_attrSFormat = flag; }
    فراغ attr_ورهايو_Var(bool flag) { m_attr_ورهايو_Var = flag; }
    فراغ attrFsmState(bool flag) { m_attrFsmState = flag; }
    فراغ attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    فراغ attrFsmStateExpand(ثابت VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool const_Pool_داخلا() ثابت { أرجع m_const_Pool_داخلا; }
    فراغ سيٽ_Const_Pool_داخلا() { m_const_Pool_داخلا = true; }
    bool covergroupRefMember() ثابت { أرجع m_covergroupRefMember; }
    فراغ covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ثابت { أرجع m_embeddedCovergroup; }
    فراغ embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    فراغ rand(ثابت VRandAttr flag) { m_rand = flag; }
    فراغ استعمال_ٿيل_Param(bool flag) { m_استعمال_ٿيل_Param = flag; }
    فراغ استعمال_ٿيل_Loop_Idx(bool flag) { m_استعمال_ٿيل_Loop_Idx = flag; }
    فراغ sig_عوامي(bool flag) { m_sig_عوامي = flag; }
    فراغ sig_Mod_عوامي(bool flag) { m_sig_Mod_عوامي = flag; }
    فراغ sig_User_Rd_عوامي(bool flag) {
        m_sig_User_Rd_عوامي = flag;
        إذا (flag) sig_عوامي(true);
    }
    فراغ sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        إذا (flag) sig_User_Rd_عوامي(true);
    }
    فراغ sc(bool flag) { m_sc = flag; }
    فراغ scSensitive(bool flag) { m_scSensitive = flag; }
    فراغ primaryIO(bool flag) { m_primaryIO = flag; }
    فراغ isConst(bool flag) { m_isConst = flag; }
    فراغ isContinuously(bool flag) { m_isContinuously = flag; }
    فراغ is_Iface_والد(bool flag) { m_is_Iface_والد = flag; }
    فراغ is_Iface_Array_ورهايو(bool flag) { m_is_Iface_Array_ورهايو = flag; }
    فراغ is_اندروني(bool flag) { m_is_اندروني = flag; }
    فراغ isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    فراغ funcLocal(bool flag) {
        m_funcLocal = flag;
        إذا (flag) m_funcLocalSticky = true;
    }
    فراغ funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ثابت { أرجع m_gotNansiType; }
    فراغ gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ثابت { أرجع m_hasStrengthAssignment; }
    فراغ hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ثابت { أرجع m_hasUserInit; }
    فراغ hasUserInit(bool flag) { m_hasUserInit = flag; }
    فراغ icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ثابت { أرجع m_icoMaybeWritten; }
    فراغ isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ثابت { أرجع m_isClockingDriven; }
    bool isDpiOpenArray() ثابت VL_MT_محفوظ { أرجع m_isDpiOpenArray; }
    فراغ isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ثابت { أرجع m_isHideLocal; }
    فراغ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ثابت { أرجع m_isHideProtected; }
    فراغ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ثابت { أرجع m_noCReset; }
    فراغ noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ثابت { أرجع m_noReset; }
    فراغ noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ثابت { أرجع m_noSubst; }
    فراغ noSubst(bool flag) { m_noSubst = flag; }
    bool process_قطار() ثابت { أرجع m_process_قطار; }
    فراغ process_قطار(bool flag) { m_process_قطار = flag; }
    bool sampled() ثابت { أرجع m_sampled; }
    فراغ sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ثابت { أرجع m_substConstOnly; }
    فراغ substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ثابت { أرجع m_overriddenParam; }
    فراغ overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ثابت { أرجع m_untypedParam; }
    فراغ untypedParam(bool flag) { m_untypedParam = flag; }
    فراغ trace(bool flag) { m_trace = flag; }
    فراغ isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ثابت { أرجع m_isForceable; }
    فراغ سيٽ_Forceable() { m_isForceable = true; }
    فراغ سيٽ_Forced_By_ڪوڊ() { m_is_Forced_By_ڪوڊ = true; }
    bool isForced() ثابت { أرجع m_isForceable || m_is_Forced_By_ڪوڊ; }
    bool isReadByDpi() ثابت { أرجع m_isReadByDpi; }
    فراغ سيٽ_Read_By_Dpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ثابت { أرجع m_isWrittenByDpi; }
    فراغ سيٽ_Written_By_Dpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ثابت { أرجع m_isWrittenBySuspendable; }
    فراغ سيٽ_Written_By_Suspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ثابت { أرجع m_ignorePostRead; }
    فراغ سيٽ_Ignore_Post_Read() { m_ignorePostRead = true; }
    bool ignorePostWrite() ثابت { أرجع m_ignorePostWrite; }
    فراغ سيٽ_Ignore_Post_Write() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ثابت { أرجع m_ignoreSchedWrite; }
    فراغ سيٽ_Ignore_Sched_Write() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ثابت { أرجع m_dfgTriLowered; }
    فراغ سيٽ_Dfg_Tri_Lowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ثابت { أرجع m_dfgAllowMultidriveTri; }
    فراغ سيٽ_Dfg_Allow_Multidrive_Tri() { m_dfgAllowMultidriveTri = true; }
    فراغ globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ثابت { أرجع m_globalConstrained; }
    bool isStdRandomizeArg() ثابت { أرجع m_isStdRandomizeArg; }
    فراغ سيٽ_Std_Randomize_Arg() { m_isStdRandomizeArg = true; }
    // METHODS
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ ٽيگ(ثابت string& متن) override { m_ٽيگ = متن; }
    string ٽيگ() ثابت override { أرجع m_ٽيگ; }
    bool isAnsi() ثابت { أرجع m_ansi; }
    bool isContinuously() ثابت { أرجع m_isContinuously; }
    bool isDeclTyped() ثابت { أرجع m_declTyped; }
    bool isInout() ثابت { أرجع m_direction.isInout(); }
    bool isInoutOrRef() ثابت { أرجع m_direction.isInoutOrRef(); }
    bool is_داخلا() ثابت { أرجع m_direction.is_داخلا(); }
    bool isNonOutput() ثابت { أرجع m_direction.isNonOutput(); }
    bool isReadOnly() ثابت VL_MT_محفوظ { أرجع m_direction.isReadOnly(); }
    bool isConstRef() ثابت VL_MT_محفوظ { أرجع m_direction.isConstRef(); }
    bool isRef() ثابت VL_MT_محفوظ { أرجع m_direction.isRef(); }
    bool isWritable() ثابت VL_MT_محفوظ { أرجع m_direction.isWritable(); }
    bool isTristate() ثابت { أرجع m_tristate; }
    bool isPrimaryIO() ثابت VL_MT_محفوظ { أرجع m_primaryIO; }
    bool isPrimaryInish() ثابت { أرجع isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ثابت VL_MT_محفوظ { أرجع m_primaryClock; }
    فراغ سيٽ_Primary_Clock() { m_primaryClock = true; }
    bool isIfaceRef() ثابت { أرجع varType() == VVarType::IFACEREF; }
    فراغ سيٽ_Iface_Ref() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool is_Iface_والد() ثابت { أرجع m_is_Iface_والد; }
    bool is_Iface_Array_ورهايو() ثابت { أرجع m_is_Iface_Array_ورهايو; }
    bool is_اندروني() ثابت { أرجع m_is_اندروني; }
    bool isIfaceParam() ثابت { أرجع m_isIfaceParam; }
    bool isSignal() ثابت { أرجع varType().isSignal(); }
    bool isNet() ثابت { أرجع varType().isNet(); }
    bool isWor() ثابت { أرجع varType().isWor(); }
    bool isWiredNet() ثابت { أرجع varType().isWiredNet(); }
    bool is_عارضي() ثابت { أرجع varType().is_عارضي(); }
    bool isClassMember() ثابت { أرجع varType() == VVarType::MEMBER; }
    bool isVirtIface() ثابت {
        إذا (AstIfaceRefDType* ثابت dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            أرجع dtp->isVirtual();
        }
        أرجع false;
    }
    bool is_Statement_عارضي() ثابت { أرجع varType() == VVarType::STMTTEMP; }
    bool isXTemp() ثابت { أرجع varType() == VVarType::XTEMP; }
    bool isParam() ثابت { أرجع varType().isParam(); }
    bool isGParam() ثابت { أرجع varType() == VVarType::GPARAM; }
    bool isGenVar() ثابت { أرجع varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ثابت {
        أرجع isIO() || isSignal() || isClassMember() || is_عارضي() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ثابت {
        أرجع !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool is_ٻٽو_انگ_Logic() ثابت {
        ثابت AstBasicDType* ثابت bdtypep = basicp();
        أرجع bdtypep && bdtypep->is_ٻٽو_انگ_Logic();
    }
    bool is_استعمال_ٿيل_Param() ثابت { أرجع m_استعمال_ٿيل_Param; }
    bool is_استعمال_ٿيل_Loop_Idx() ثابت { أرجع m_استعمال_ٿيل_Loop_Idx; }
    bool mtask_Cache_سٽ_Align() ثابت { أرجع m_mtask_Cache_سٽ_Align; }
    فراغ mtask_Cache_سٽ_Align(bool flag) { m_mtask_Cache_سٽ_Align = flag; }
    bool isSc() ثابت VL_MT_محفوظ { أرجع m_sc; }
    bool isScQuad() ثابت;
    bool isScBv() ثابت VL_MT_STABLE;
    bool isScUint() ثابت;
    bool isScUintBool() ثابت;
    bool isScBigUint() ثابت VL_MT_STABLE;
    bool isScSensitive() ثابت { أرجع m_scSensitive; }
    bool is_Sig_عوامي() ثابت;
    bool is_Sig_Mod_عوامي() ثابت { أرجع m_sig_Mod_عوامي && !isIfaceRef(); }
    bool is_Sig_User_Rd_عوامي() ثابت { أرجع m_sig_User_Rd_عوامي && !isIfaceRef(); }
    bool isSigUserRWPublic() ثابت { أرجع m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ثابت { أرجع m_trace; }
    bool isRand() ثابت { أرجع m_rand.isRand(); }
    bool isRandC() ثابت { أرجع m_rand.isRandC(); }
    bool isConst() ثابت VL_MT_محفوظ { أرجع m_isConst; }
    bool isLatched() ثابت { أرجع m_isLatched; }
    bool isFuncLocal() ثابت { أرجع m_funcLocal; }
    bool isFuncLocalSticky() ثابت { أرجع m_funcLocalSticky; }
    bool isFuncReturn() ثابت { أرجع m_funcReturn; }
    bool isPullup() ثابت { أرجع m_isPullup; }
    bool isPulldown() ثابت { أرجع m_isPulldown; }
    bool attrScBv() ثابت { أرجع m_attrScBv; }
    bool attrScBigUint() ثابت { أرجع m_attrScBigUint; }
    bool attrFileDescr() ثابت { أرجع m_attrFileDescr; }
    bool attrSFormat() ثابت { أرجع m_attrSFormat; }
    bool attr_ورهايو_Var() ثابت { أرجع m_attr_ورهايو_Var; }
    bool attrFsmState() ثابت { أرجع m_attrFsmState; }
    bool attrFsmArcInclCond() ثابت { أرجع m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ثابت { أرجع m_attrFsmStateExpand; }
    AstIface* sensIfacep() ثابت { أرجع m_sensIfacep; }
    VRandAttr rand() ثابت { أرجع m_rand; }
    string verilogKwd() ثابت override;
    فراغ lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ثابت { أرجع m_lifetime; }
    فراغ pin_انگ(صحيح id) { m_pin_انگ = id; }
    صحيح pin_انگ() ثابت { أرجع m_pin_انگ; }
    فراغ propagateAttrFrom(ثابت AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        إذا (fromp->attrFileDescr()) attrFileDescr(true);
        إذا (fromp->isContinuously()) isContinuously(true);
    }
    فراغ propagate_ورهايو_Attr_From(ثابت AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attr_ورهايو_Var(fromp->attr_ورهايو_Var());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ثابت حرف* cannot_ورهايو_قسم_Reason() ثابت;
    فراغ propagateWrapAttrFrom(ثابت AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    فراغ combineType(ثابت AstVar* otherp);
    bool needsCReset() ثابت {
        أرجع !is_Iface_والد() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_عارضي()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    ساكن AstVar* scVarRecurse(Ast_ڳنڍ* nodep);
};
class AstVarScope final : عام Ast_ڳنڍ {
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
عام:
    AstVarScope(File_سٽ* fl, AstScope* scopep, AstVar* varp)
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
    فراغ cloneRelink() override {
        إذا (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    string name() ثابت override VL_MT_STABLE { أرجع scopep()->name() + "->" + varp()->name(); }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    AstVar* varp() ثابت VL_MT_STABLE { أرجع m_varp; }  // [After Link] Pointer to variable
    فراغ varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ثابت VL_MT_STABLE { أرجع m_scopep; }  // Pointer to scope it's under
    فراغ scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ثابت { أرجع m_trace; }
    فراغ trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ثابت { أرجع m_optimizeLifePost; }
    فراغ optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_قدر() ثابت { أرجع m_fork_By_قدر; }
    فراغ fork_By_قدر(bool flag) { m_fork_By_قدر = flag; }
};

// === AstNodeConsItem ===
class Ast_Cons_Array_شي final : عام Ast_ڳنڍ_Cons_شي {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ثابت uint64_t m_اشاريو;  // Index of the element, with the array's lo() at 0
عام:
    Ast_Cons_Array_شي(File_سٽ* fl, uint64_t اشاريو, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cons_Array_شي(fl, valuep)
        , m_اشاريو{اشاريو} {}
    ASTGEN_MEMBERS_Ast_Cons_Array_شي;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), Cons_Array_شي));
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        أرجع m_اشاريو == VN_DBG_AS(samep, Cons_Array_شي)->m_اشاريو;
    }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }  // By AstConsArray's map
    uint64_t اشاريو() ثابت { أرجع m_اشاريو; }
};
class Ast_Cons_Assoc_شي final : عام Ast_ڳنڍ_Cons_شي {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
عام:
    Ast_Cons_Assoc_شي(File_سٽ* fl, Ast_ڳنڍ_اظهار* keyp, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cons_Assoc_شي(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Assoc_شي;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), Cons_Assoc_شي));
        أرجع nullptr;
    }
};
class Ast_Cons_Dyn_Array_شي final : عام Ast_ڳنڍ_Cons_شي {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
عام:
    Ast_Cons_Dyn_Array_شي(File_سٽ* fl, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_شي(fl, valuep) {}
    Ast_Cons_Dyn_Array_شي(File_سٽ* fl, AstConst* keyp, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_شي(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Dyn_Array_شي;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), Cons_Dyn_Array_شي));
        أرجع nullptr;
    }
};
class Ast_Cons_قطار_شي final : عام Ast_ڳنڍ_Cons_شي {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
عام:
    Ast_Cons_قطار_شي(File_سٽ* fl, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cons_قطار_شي(fl, valuep) {}
    ASTGEN_MEMBERS_Ast_Cons_قطار_شي;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), Cons_قطار) && !VN_IS(backp(), Cons_قطار_شي));
        أرجع nullptr;
    }
};
class Ast_Cons_Struct_شي final : عام Ast_ڳنڍ_Cons_شي {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
عام:
    Ast_Cons_Struct_شي(File_سٽ* fl, AstMemberDType* memberp, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cons_Struct_شي(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_Ast_Cons_Struct_شي;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), Cons_Struct_شي));
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    AstMemberDType* memberp() ثابت { أرجع m_memberp; }
};
class Ast_Cons_Wildcard_شي final : عام Ast_ڳنڍ_Cons_شي {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
عام:
    Ast_Cons_Wildcard_شي(File_سٽ* fl, AstConst* keyp, Ast_ڳنڍ_اظهار* valuep)
        : ASTGEN_SUPER_Cons_Wildcard_شي(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Wildcard_شي;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), Cons_Wildcard_شي));
        أرجع nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : عام Ast_ڳنڍ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsm_منزل;  // FSM destination state label
    string m_fsm_ٽيگ;  // FSM arc kind tag (e.g. reset, reset_include, default)
    صحيح m_وٿي;  // Offset column numbers to uniq-ify IFs
عام:
    AstCoverOtherDecl(File_سٽ* fl, ثابت string& صفحو, ثابت string& comment,
                      ثابت string& linescov, صحيح وٿي, ثابت string& fsmVar = "",
                      ثابت string& fsmFrom = "", ثابت string& fsm_منزل = "",
                      ثابت string& fsm_ٽيگ = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, صفحو, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsm_منزل{fsm_منزل}
        , m_fsm_ٽيگ{fsm_ٽيگ}
        , m_وٿي{وٿي} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح وٿي() ثابت { أرجع m_وٿي; }
    صحيح سائيز() ثابت override { أرجع 1; }
    ثابت string& linescov() ثابت { أرجع m_linescov; }
    ثابت string& fsmVar() ثابت { أرجع m_fsmVar; }
    ثابت string& fsmFrom() ثابت { أرجع m_fsmFrom; }
    ثابت string& fsm_منزل() ثابت { أرجع m_fsm_منزل; }
    ثابت string& fsm_ٽيگ() ثابت { أرجع m_fsm_ٽيگ; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstCoverOtherDecl* ثابت asamep = VN_DBG_AS(samep, CoverOtherDecl);
        أرجع Super::same_ڳنڍ(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : عام Ast_ڳنڍ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ثابت VNumRange m_range;  // Packed array range covering each toggle bit
عام:
    AstCoverToggleDecl(File_سٽ* fl, ثابت string& صفحو, ثابت string& comment,
                       ثابت VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, صفحو, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح سائيز() ثابت override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        أرجع 2 * m_range.elements();
    }
    ثابت VNumRange& range() ثابت { أرجع m_range; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstCoverToggleDecl* ثابت asamep = VN_DBG_AS(samep, CoverToggleDecl);
        أرجع Super::same_ڳنڍ(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : عام Ast_ڳنڍ_FTask {
    // A function inside a module
عام:
    AstFunc(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* stmtp, Ast_ڳنڍ* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    Ast_ڳنڍ_FTask* cloneType(ثابت string& name) override {
        أرجع new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "function"; }
};
class AstLet final : عام Ast_ڳنڍ_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
عام:
    AstLet(File_سٽ* fl, ثابت string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_اظهار));
        أرجع nullptr;
    }
    Ast_ڳنڍ_FTask* cloneType(ثابت string& name) override { أرجع new AstLet{fileline(), name}; }
    string verilogKwd() ثابت override { أرجع "let"; }
};
class AstProperty final : عام Ast_ڳنڍ_FTask {
    // A property inside a module
عام:
    AstProperty(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    Ast_ڳنڍ_FTask* cloneType(ثابت string& name) override {
        أرجع new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "property"; }
};
class AstSequence final : عام Ast_ڳنڍ_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
عام:
    AstSequence(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_محفوظ { أرجع true; }
    Ast_ڳنڍ_FTask* cloneType(ثابت string& name) override {
        أرجع new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "sequence"; }
    bool isReferenced() ثابت { أرجع m_referenced; }
    فراغ isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : عام Ast_ڳنڍ_FTask {
    // A task inside a module
عام:
    AstTask(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_ڳنڍ_FTask* cloneType(ثابت string& name) override {
        أرجع new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "task"; }
};

// === AstNodeFile ===
class AstCFile final : عام Ast_ڳنڍ_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_ذريعو : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
عام:
    AstCFile(File_سٽ* fl, ثابت string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_ذريعو{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    uint64_t complexityScore() ثابت { أرجع m_complexityScore; }
    فراغ complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ثابت { أرجع m_slow; }
    فراغ slow(bool flag) { m_slow = flag; }
    bool ذريعو() ثابت { أرجع m_ذريعو; }
    فراغ ذريعو(bool flag) { m_ذريعو = flag; }
    bool support() ثابت { أرجع m_support; }
    فراغ support(bool flag) VL_MT_محفوظ { m_support = flag; }
};
class AstVFile final : عام Ast_ڳنڍ_File {
    // Verilog output file
    // Parents:  NETLIST
عام:
    AstVFile(File_سٽ* fl, ثابت string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstCoverCross final : عام Ast_ڳنڍ_Func_Cov_شي {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
عام:
    AstCoverCross(File_سٽ* fl, ثابت string& name, AstCoverpointRef* itemsp,
                  Ast_ڳنڍ_اظهار* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        جوڙيو_Itemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstCoverpoint final : عام Ast_ڳنڍ_Func_Cov_شي {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
عام:
    AstCoverpoint(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ_اظهار* exprp,
                  Ast_ڳنڍ_اظهار* iffp = nullptr, Ast_ڳنڍ* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        جوڙيو_Binsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};

// === AstNodeGen ===
class Ast_Gen_بلاڪ final : عام Ast_ڳنڍ_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ثابت bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ثابت bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

عام:
    Ast_Gen_بلاڪ(File_سٽ* fl, ثابت string& name, Ast_ڳنڍ* itemsp, bool implied)
        : ASTGEN_SUPER_Gen_بلاڪ(fl)
        , m_name{name}
        , m_unnamed{name.خالي_جايون()}
        , m_implied{implied} {
        this->جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_بلاڪ;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت std::string& name) override { m_name = name; }
    bool unnamed() ثابت { أرجع m_unnamed; }
    bool implied() ثابت { أرجع m_implied; }
    AstDefaultDisable* defaultDisablep() ثابت { أرجع m_defaultDisablep; }
    فراغ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ثابت string& outer) ثابت;
};
class AstGenCase final : عام Ast_ڳنڍ_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
عام:
    AstGenCase(File_سٽ* fl, Ast_ڳنڍ_اظهار* exprp, Ast_Gen_Case_شي* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : عام Ast_ڳنڍ_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
عام:
    AstGenFor(File_سٽ* fl, Ast_ڳنڍ* initsp, Ast_ڳنڍ_اظهار* condp, Ast_ڳنڍ* incsp, Ast_ڳنڍ* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->جوڙيو_Initsp(initsp);
        this->condp(condp);
        this->جوڙيو_Incsp(incsp);
        this->جوڙيو_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : عام Ast_ڳنڍ_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
عام:
    AstGenIf(File_سٽ* fl, Ast_ڳنڍ_اظهار* condp, Ast_ڳنڍ* thensp, Ast_ڳنڍ* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->جوڙيو_Thensp(thensp);
        this->جوڙيو_Elsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : عام Ast_ڳنڍ_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtype_Name_ڀريل;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_انگ;  // Declaration token number
    VBaseOverride m_بنياد_Override;  // BaseOverride (inital/final/extends)
    bool m_hasRandVarsUpdate = false;  // Has updateRandVars method,
                                       // which updates pointers to rand variables in clone()
    bool m_covergroup = false;  // Is covergroup (TODO perhaps make a new Ast node type for CG?)
    bool m_extended = false;  // Is extension or extended by other classes
    bool m_interfaceClass = false;  // Interface class
    bool m_needRNG = false;  // Need RNG, uses srandom/randomize
    bool m_use_Virtual_عوامي = false;  // Subclasses need virtual public as uses interface class
    bool m_virtual = false;  // Virtual class
    bool m_printedFrom = false;  // This class is printed from i.e. is used as format arg.
    // Mutable, as only a recursion guard of the const dtypeName(), so also not dumped
    mutable bool m_dtypeNameBusy = false;  // In dtypeName(), which a parameter may lead back to
    // Covergroup options (when m_covergroup is true)
    صحيح m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool ڀريل_2, ثابت string& scope) ثابت;
    string dtypeNameScope() ثابت;  // Prefix of dtypeName(true) for the scope declaring the class

عام:
    AstClass(File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_انگ{fl->token_انگ()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ثابت override { أرجع isCovergroup() ? "covergroup" : "class"; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool timescaleMatters() ثابت override { أرجع false; }
    AstClassPackage* classOrPackagep() ثابت VL_MT_STABLE { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ثابت VL_MT_STABLE {
        أرجع m_covergroupEnclosingClassp;
    }
    فراغ covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_ڳنڍ* membersp() ثابت VL_MT_STABLE { أرجع stmtsp(); }
    فراغ جوڙيو_Membersp(Ast_ڳنڍ* nodep) { جوڙيو_Stmtsp(nodep); }
    bool hasRandVarsUpdate() ثابت { أرجع m_hasRandVarsUpdate; }
    فراغ hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ثابت { أرجع m_covergroup; }
    فراغ isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ثابت { أرجع m_extended; }
    فراغ isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ثابت { أرجع m_interfaceClass; }
    فراغ isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ثابت { أرجع m_virtual; }
    فراغ isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ثابت { أرجع m_needRNG; }
    فراغ needRNG(bool flag) { m_needRNG = flag; }
    bool use_Virtual_عوامي() ثابت { أرجع m_use_Virtual_عوامي; }
    فراغ use_Virtual_عوامي(bool flag) { m_use_Virtual_عوامي = flag; }
    فراغ markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ثابت { أرجع m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool ڀريل_2) ثابت;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtype_Name_اندريون(ثابت string& scope) ثابت;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    فراغ dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ثابت { أرجع !m_dtype_Name_ڀريل.خالي_جايون(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_نياپو() ثابت override { أرجع hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    ساكن string embeddedCovergroupTypeName(ثابت string& name) { أرجع "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ثابت;
    // Covergroup options accessors
    صحيح cgAutoBinMax() ثابت { أرجع m_cgAutoBinMax; }
    فراغ cgAutoBinMax(صحيح قدر) { m_cgAutoBinMax = قدر; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    ساكن bool isClassExtendedFrom(ثابت AstClass* refClassp, ثابت AstClass* بنياد_Classp);
    uint32_t decl_Token_انگ() ثابت override { أرجع m_decl_Token_انگ; }
    فراغ decl_Token_انگ_سيٽ_Min(uint32_t token_انگ) override {
        m_decl_Token_انگ = std::min(m_decl_Token_انگ, token_انگ);
    }
    فراغ بنياد_Override(ثابت VBaseOverride& flag) { m_بنياد_Override = flag; }
    VBaseOverride بنياد_Override() ثابت { أرجع m_بنياد_Override; }
    // Return the lowest class extended from, or this class
    AstClass* بنياد_Most_Classp();
    ساكن bool is_Cacheable_ٻار(ثابت Ast_ڳنڍ* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    سانچو <typename T_Callable>
    فراغ foreachMember(ثابت T_Callable& f) {
        using T_ڳنڍ = typename Function_Arg_No_اشارو_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_ڳنڍ*>::قدر
                && std::is_بنياد_جو<Ast_ڳنڍ, T_ڳنڍ>::قدر,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        إذا (ثابت AstClassExtends* ثابت cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        لكل (Ast_ڳنڍ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_ڳنڍ::ذاتي_Type_Test<T_ڳنڍ>(stmtp)) f(this, static_cast<T_ڳنڍ*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    سانچو <typename T_Callable>
    bool existsMember(ثابت T_Callable& p) ثابت {
        using T_ڳنڍ = typename Function_Arg_No_اشارو_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ثابت AstClass*, ثابت T_ڳنڍ*>::قدر
                && std::is_بنياد_جو<Ast_ڳنڍ, T_ڳنڍ>::قدر,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        إذا (ثابت AstClassExtends* ثابت cextendsp = this->extendsp()) {
            إذا (cextendsp->classp()->existsMember(p)) أرجع true;
        }
        لكل (Ast_ڳنڍ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_ڳنڍ::ذاتي_Type_Test<T_ڳنڍ>(stmtp)) {
                إذا (p(this, static_cast<T_ڳنڍ*>(stmtp))) أرجع true;
            }
        }
        أرجع false;
    }
};
class AstClassPackage final : عام Ast_ڳنڍ_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
عام:
    AstClassPackage(File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ثابت override { أرجع "classpackage"; }
    bool timescaleMatters() ثابت override { أرجع false; }
    AstClass* classp() ثابت VL_MT_محفوظ { أرجع m_classp; }
    فراغ classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : عام Ast_ڳنڍ_Module {
    // An interface declaration
    string m_dtype_Name_ڀريل;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
عام:
    AstIface(File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { أرجع "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ثابت override { أرجع false; }
    bool hasVirtualRef() ثابت { أرجع m_hasVirtualRef; }
    فراغ سيٽ_Has_Virtual_Ref() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool ڀريل_2) ثابت;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    فراغ dtypeNameFreeze();
};
class AstModule final : عام Ast_ڳنڍ_Module {
    // A module declaration
    ثابت bool m_isChecker;  // Module represents a checker
    ثابت bool m_is_پروگرام;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

عام:
    class Checker {};  // for constructor type-overload selection
    class پروگرام {};  // for constructor type-overload selection
    AstModule(File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_پروگرام{false} {}
    AstModule(File_سٽ* fl, ثابت string& name, ثابت string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_is_پروگرام{false} {}
    AstModule(File_سٽ* fl, ثابت string& name, ثابت string& libname, پروگرام)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_پروگرام{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ثابت override {
        أرجع m_isChecker ? "checker" : m_is_پروگرام ? "program" : "module";
    }
    bool timescaleMatters() ثابت override { أرجع true; }
    bool isChecker() ثابت { أرجع m_isChecker; }
    bool is_پروگرام() ثابت { أرجع m_is_پروگرام; }
    bool hasGenericIface() ثابت { أرجع m_hasGenericIface; }
    فراغ hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class Ast_Not_مليو_Module final : عام Ast_ڳنڍ_Module {
    // A missing module declaration
عام:
    Ast_Not_مليو_Module(File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Not_مليو_Module(fl, name, libname) {}
    ASTGEN_MEMBERS_Ast_Not_مليو_Module;
    string verilogKwd() ثابت override { أرجع "/*not-found-*/ module"; }
    bool timescaleMatters() ثابت override { أرجع false; }
};
class AstPackage final : عام Ast_ڳنڍ_Module {
    // A package declaration
عام:
    AstPackage(File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ثابت override { أرجع "package"; }
    bool timescaleMatters() ثابت override { أرجع !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : عام Ast_ڳنڍ_Module {
    // A primitive declaration
عام:
    AstPrimitive(File_سٽ* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ثابت override { أرجع "primitive"; }
    bool timescaleMatters() ثابت override { أرجع false; }
};

// === AstNodeProcedure ===
class AstAlways final : عام Ast_ڳنڍ_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ثابت VAlwaysKwd m_keyword;

عام:
    AstAlways(File_سٽ* fl, VAlwaysKwd keyword, Ast_Sen_وڻ* sentreep, Ast_ڳنڍ* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit مضمن AstAlways(Ast_سونپيو_W* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VAlwaysKwd keyword() ثابت { أرجع m_keyword; }
};
class AstAlwaysObserved final : عام Ast_ڳنڍ_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

عام:
    AstAlwaysObserved(File_سٽ* fl, Ast_Sen_وڻ* sentreep, Ast_ڳنڍ* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : عام Ast_ڳنڍ_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

عام:
    explicit AstAlwaysPost(File_سٽ* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : عام Ast_ڳنڍ_Procedure {
    // Like always but Postponed scheduling region

عام:
    AstAlwaysPostponed(File_سٽ* fl, Ast_ڳنڍ* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : عام Ast_ڳنڍ_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

عام:
    explicit AstAlwaysPre(File_سٽ* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : عام Ast_ڳنڍ_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

عام:
    AstAlwaysReactive(File_سٽ* fl, Ast_Sen_وڻ* sentreep, Ast_ڳنڍ* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : عام Ast_ڳنڍ_Procedure {
عام:
    AstFinal(File_سٽ* fl, Ast_ڳنڍ* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : عام Ast_ڳنڍ_Procedure {
عام:
    AstInitial(File_سٽ* fl, Ast_ڳنڍ* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : عام Ast_ڳنڍ_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
عام:
    AstInitialAutomatic(File_سٽ* fl, Ast_ڳنڍ* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : عام Ast_ڳنڍ_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
عام:
    AstInitialStatic(File_سٽ* fl, Ast_ڳنڍ* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : عام Ast_ڳنڍ_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
عام:
    AstBracketRange(File_سٽ* fl, Ast_ڳنڍ* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    مجازي string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_منزل() ثابت override VL_MT_محفوظ { أرجع false; }
};
class AstRange final : عام Ast_ڳنڍ_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ثابت bool m_fromBracket = false;  // From C-style '[x]' declaration
عام:
    AstRange(File_سٽ* fl, Ast_ڳنڍ_اظهار* leftp, Ast_ڳنڍ_اظهار* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    مضمن AstRange(File_سٽ* fl, صحيح کاٻو, صحيح ساڄو);
    مضمن AstRange(File_سٽ* fl, ثابت VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    مضمن صحيح کاٻو_Const() ثابت VL_MT_STABLE;
    مضمن صحيح ساڄو_Const() ثابت VL_MT_STABLE;
    صحيح hiConst() ثابت VL_MT_STABLE {
        ثابت صحيح l = کاٻو_Const();
        ثابت صحيح r = ساڄو_Const();
        أرجع l > r ? l : r;
    }
    صحيح loConst() ثابت VL_MT_STABLE {
        ثابت صحيح l = کاٻو_Const();
        ثابت صحيح r = ساڄو_Const();
        أرجع l > r ? r : l;
    }
    صحيح elementsConst() ثابت VL_MT_STABLE { أرجع hiConst() - loConst() + 1; }
    bool ascending() ثابت { أرجع کاٻو_Const() < ساڄو_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_کاٻو_منزل_ساڄو() ثابت {
        أرجع VNumRange{کاٻو_Const(), ساڄو_Const()}.seq_کاٻو_منزل_ساڄو();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_ساڄو_منزل_کاٻو() ثابت {
        أرجع VNumRange{کاٻو_Const(), ساڄو_Const()}.seq_ساڄو_منزل_کاٻو();
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* samep) ثابت override {
        ثابت AstRange* ثابت asamep = VN_DBG_AS(samep, Range);
        أرجع fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ثابت { أرجع m_fromBracket; }
};
class AstUnsizedRange final : عام Ast_ڳنڍ_Range {
    // Unsized range specification, for open arrays
عام:
    explicit AstUnsizedRange(File_سٽ* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    مجازي string emitVerilog() { أرجع "[]"; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};
class AstWildcardRange final : عام Ast_ڳنڍ_Range {
    // Wildcard range specification, for wildcard index type associative arrays
عام:
    explicit AstWildcardRange(File_سٽ* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    مجازي string emitVerilog() { أرجع "[*]"; }
    bool same_ڳنڍ(ثابت Ast_ڳنڍ* /*samep*/) ثابت override { أرجع true; }
};

#جيڪڏهن_پورو  // Guard
