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

#నిర్వచితం_కాకపోతే VERILATOR_V3ASTNODES_H_
#నిర్వచించు VERILATOR_V3ASTNODES_H_

#నిర్వచితం_కాకపోతే VERILATOR_V3AST_H_
#దోషం "Use V3Ast.h as the include"
#అన్తర్భావ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#నిర్వచించు VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#అయితే_ముగింపు

#అన్తర్భావ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_కణుపు_Cons_అంశం VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    మిత్ర class Ast_కణుపు_సమాసం;  // For m_knownConstantTree
    mutable bool m_తెలిసిన_Constant_వృక్షం = false;  // Known to be a tree of constants

రక్షిత:
    Ast_కణుపు_Cons_అంశం(VNType t, File_పంక్తి* fl, Ast_కణుపు_సమాసం* valuep)
        : Ast_కణుపు{t, fl} {
        this->valuep(valuep);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Cons_అంశం;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class Ast_కణుపు_Cover_Decl VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_పుట;  // Coverage point's page tag
    string m_పాఠ్యం;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    పూర్ణాంకం m_bin_అంకె = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    పూర్ణాంకం m_local_Bin_అంకె = 0;  // Per-module coverage bin offset
    bool m_చొప్పున_Instance = false;  // Don't clone during inlining, already per instance
బహిరంగ:
    Ast_కణుపు_Cover_Decl(VNType t, File_పంక్తి* fl, స్థిరరాశి string& పుట, స్థిరరాశి string& comment)
        : Ast_కణుపు(t, fl)
        , m_పుట{పుట}
        , m_పాఠ్యం{comment} {}
    ASTGEN_MEMBERS_Ast_కణుపు_Cover_Decl;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        ఒకవేళ (m_దత్తాంశం_Declp
            && (m_దత్తాంశం_Declp == this || m_దత్తాంశం_Declp->m_దత్తాంశం_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvt_గమ్యం_Hex(this));
        }
        తిరిగి nullptr;
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 1 + 2 * INSTR_లెక్క_LD; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    పూర్ణాంకం bin_అంకె() స్థిరరాశి { తిరిగి m_bin_అంకె; }
    శూన్య bin_అంకె(పూర్ణాంకం flag) { m_bin_అంకె = flag; }
    పూర్ణాంకం local_Bin_అంకె() స్థిరరాశి { తిరిగి m_local_Bin_అంకె; }
    శూన్య local_Bin_అంకె(పూర్ణాంకం flag) { m_local_Bin_అంకె = flag; }
    bool చొప్పున_Instance() స్థిరరాశి { తిరిగి m_చొప్పున_Instance; }
    శూన్య చొప్పున_Instance(bool flag) { m_చొప్పున_Instance = flag; }
    మాయా పూర్ణాంకం కొలత() స్థిరరాశి = 0;
    స్థిరరాశి string& comment() స్థిరరాశి { తిరిగి m_పాఠ్యం; }  // text to insert in code
    స్థిరరాశి string& పుట() స్థిరరాశి { తిరిగి m_పుట; }
    స్థిరరాశి string& hier() స్థిరరాశి { తిరిగి m_hier; }
    శూన్య hier(స్థిరరాశి string& flag) { m_hier = flag; }
    శూన్య comment(స్థిరరాశి string& flag) { m_పాఠ్యం = flag; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి Ast_కణుపు_Cover_Decl* స్థిరరాశి asamep = VN_DBG_AS(samep, కణుపు_Cover_Decl);
        తిరిగి (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && పుట() == asamep->పుట()
                && చొప్పున_Instance() == asamep->చొప్పున_Instance());
    }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    శూన్య దత్తాంశం_Declp(Ast_కణుపు_Cover_Decl* nodep) { m_దత్తాంశం_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_కణుపు_Cover_Decl* దత్తాంశం_Decl_Nullp() స్థిరరాశి { తిరిగి m_దత్తాంశం_Declp; }
    Ast_కణుపు_Cover_Decl* దత్తాంశం_Decl_Thisp() { తిరిగి దత్తాంశం_Decl_Nullp() ? దత్తాంశం_Decl_Nullp() : this; }
};
class Ast_కణుపు_FTask VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
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
    string m_iface_పోర్టు_Name;  // Interface port name for out-of-block definition (IEEE 25.8)
    uint64_t m_dpi_Open_మాతృ = 0;  // DPI import open array, if !=0, how many callees
    bool m_task_ప్రజా : 1;  // Public task
    bool m_class_పద్ధతి : 1;  // Class method
    bool m_didProto : 1;  // Did prototype processing
    bool m_prototype : 1;  // Just a prototype
    bool m_dpiExport : 1;  // DPI exported
    bool m_dpiImport : 1;  // DPI imported
    bool m_dpiContext : 1;  // DPI import context
    bool m_dpi_Open_బిడ్డ : 1;  // DPI import open array child wrapper
    bool m_dpiTask : 1;  // DPI import task (vs. void function)
    bool m_isConstructor : 1;  // Class constructor
    bool m_isExternProto : 1;  // Extern prototype
    bool m_isExternDef : 1;  // Extern definition
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_dpi_శుద్ధ : 1;  // DPI import pure (vs. virtual pure)
    bool m_keepAlive : 1;  // Disable dead function elimination
    bool m_needsSyms : 1;  // Requires vlSymsp argument
    bool m_శుద్ధ_Virtual : 1;  // Pure virtual
    bool m_recursive : 1;  // Recursive or part of recursion
    bool m_static : 1;  // Static method in class
    bool m_underGenerate : 1;  // Under generate (for warning)
    bool m_verilogFunction : 1;  // Declared by user as function (versus internal-made)
    bool m_verilogTask : 1;  // Declared by user as task (versus internal-made)
    bool m_virtual : 1;  // Virtual method in class
    bool m_needProcess : 1;  // Needs access to VlProcess of the caller
    bool m_isCovergroupSample : 1;  // Covergroup sample() method
    VBaseOverride m_పునాది_Override;  // BaseOverride (inital/final/extends)
    VLifetime m_lifetime;  // Default lifetime of local vars
    VIsCached m_purity;  // Pure state

రక్షిత:
    Ast_కణుపు_FTask(VNType t, File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* stmtsp)
        : Ast_కణుపు{t, fl}
        , m_name{name}
        , m_task_ప్రజా{false}
        , m_class_పద్ధతి{false}
        , m_didProto{false}
        , m_prototype{false}
        , m_dpiExport{false}
        , m_dpiImport{false}
        , m_dpiContext{false}
        , m_dpi_Open_బిడ్డ{false}
        , m_dpiTask{false}
        , m_isConstructor{false}
        , m_isExternProto{false}
        , m_isExternDef{false}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_dpi_శుద్ధ{false}
        , m_keepAlive{false}
        , m_needsSyms{true}
        , m_శుద్ధ_Virtual{false}
        , m_recursive{false}
        , m_static{false}
        , m_underGenerate{false}
        , m_verilogFunction{false}
        , m_verilogTask{false}
        , m_virtual{false}
        , m_needProcess{false}
        , m_isCovergroupSample{false} {
        కూడు_Stmtsp(stmtsp);
        cname(name);  // Might be overridden by dpi import/export
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_FTask;
    మాయా Ast_కణుపు_FTask* cloneType(స్థిరరాశి string& name) = 0;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool isGateOptimizable() స్థిరరాశి override {
        తిరిగి !((m_dpiExport || m_dpiImport) && !m_dpi_శుద్ధ);
    }
    // {AstFunc only} op1 = Range output variable
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string cname() స్థిరరాశి { తిరిగి m_cname; }
    శూన్య cname(స్థిరరాశి string& cname) { m_cname = cname; }
    bool isFunction() స్థిరరాశి { తిరిగి fvarp() != nullptr; }
    // MORE ACCESSORS
    శూన్య dpi_Open_మాతృ_Inc() { ++m_dpi_Open_మాతృ; }
    శూన్య dpi_Open_మాతృ_Clear() { m_dpi_Open_మాతృ = 0; }
    uint64_t dpi_Open_మాతృ() స్థిరరాశి { తిరిగి m_dpi_Open_మాతృ; }
    bool task_ప్రజా() స్థిరరాశి { తిరిగి m_task_ప్రజా; }
    శూన్య task_ప్రజా(bool flag) { m_task_ప్రజా = flag; }
    bool class_పద్ధతి() స్థిరరాశి { తిరిగి m_class_పద్ధతి; }
    శూన్య class_పద్ధతి(bool flag) { m_class_పద్ధతి = flag; }
    bool didProto() స్థిరరాశి { తిరిగి m_didProto; }
    శూన్య didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() స్థిరరాశి { తిరిగి m_isExternProto; }
    శూన్య isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() స్థిరరాశి { తిరిగి m_isExternDef; }
    శూన్య isExternDef(bool flag) { m_isExternDef = flag; }
    స్థిరరాశి string& iface_పోర్టు_Name() స్థిరరాశి { తిరిగి m_iface_పోర్టు_Name; }
    శూన్య iface_పోర్టు_Name(స్థిరరాశి string& name) { m_iface_పోర్టు_Name = name; }
    bool prototype() స్థిరరాశి { తిరిగి m_prototype; }
    శూన్య prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() స్థిరరాశి { తిరిగి m_dpiExport; }
    శూన్య dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() స్థిరరాశి { తిరిగి m_dpiImport; }
    శూన్య dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() స్థిరరాశి { తిరిగి m_dpiContext; }
    శూన్య dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpi_Open_బిడ్డ() స్థిరరాశి { తిరిగి m_dpi_Open_బిడ్డ; }
    శూన్య dpi_Open_బిడ్డ(bool flag) { m_dpi_Open_బిడ్డ = flag; }
    bool dpiTask() స్థిరరాశి { తిరిగి m_dpiTask; }
    శూన్య dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() స్థిరరాశి { తిరిగి !m_dpiCDecl.ఖాళీలు(); }
    స్థిరరాశి string& dpiCDecl() స్థిరరాశి { తిరిగి m_dpiCDecl; }
    శూన్య dpiCDecl(స్థిరరాశి string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() స్థిరరాశి { తిరిగి m_isConstructor; }
    శూన్య isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() స్థిరరాశి { తిరిగి m_isHideLocal; }
    శూన్య isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() స్థిరరాశి { తిరిగి m_isHideProtected; }
    శూన్య isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpi_శుద్ధ() స్థిరరాశి { తిరిగి m_dpi_శుద్ధ; }
    శూన్య dpi_శుద్ధ(bool flag) { m_dpi_శుద్ధ = flag; }
    bool keepAlive() స్థిరరాశి { తిరిగి m_keepAlive; }
    శూన్య keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() స్థిరరాశి { తిరిగి m_needsSyms; }
    శూన్య needsSyms(bool flag) { m_needsSyms = flag; }
    bool శుద్ధ_Virtual() స్థిరరాశి { తిరిగి m_శుద్ధ_Virtual; }
    శూన్య శుద్ధ_Virtual(bool flag) { m_శుద్ధ_Virtual = flag; }
    bool recursive() స్థిరరాశి { తిరిగి m_recursive; }
    శూన్య recursive(bool flag) { m_recursive = flag; }
    bool isStatic() స్థిరరాశి { తిరిగి m_static; }
    శూన్య isStatic(bool flag) { m_static = flag; }
    bool underGenerate() స్థిరరాశి { తిరిగి m_underGenerate; }
    శూన్య underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() స్థిరరాశి { తిరిగి m_verilogFunction; }
    శూన్య verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() స్థిరరాశి { తిరిగి m_verilogTask; }
    శూన్య verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() స్థిరరాశి { తిరిగి m_virtual; }
    శూన్య isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() స్థిరరాశి { తిరిగి m_needProcess; }
    శూన్య సమితి_Need_Process() { m_needProcess = true; }
    bool isCovergroupSample() స్థిరరాశి { తిరిగి m_isCovergroupSample; }
    శూన్య isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    శూన్య పునాది_Override(స్థిరరాశి VBaseOverride& flag) { m_పునాది_Override = flag; }
    VBaseOverride పునాది_Override() స్థిరరాశి { తిరిగి m_పునాది_Override; }
    శూన్య lifetime(స్థిరరాశి VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() స్థిరరాశి { తిరిగి m_lifetime; }
    bool is_శుద్ధ() override;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య propagateAttrFrom(స్థిరరాశి Ast_కణుపు_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        class_పద్ధతి(fromp->class_పద్ధతి());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

అంతరంగ:
    bool getPurityRecurse() స్థిరరాశి;
};
class Ast_కణుపు_File VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
బహిరంగ:
    Ast_కణుపు_File(VNType t, File_పంక్తి* fl, స్థిరరాశి string& name)
        : Ast_కణుపు{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_కణుపు_File;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_కణుపు_Func_Cov_అంశం VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // Base class for functional coverage items (coverpoints, crosses)
రక్షిత:
    string m_name;  // Item name

బహిరంగ:
    Ast_కణుపు_Func_Cov_అంశం(VNType t, File_పంక్తి* fl, స్థిరరాశి string& name)
        : Ast_కణుపు{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_కణుపు_Func_Cov_అంశం;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override { తిరిగి true; }
};
class Ast_కణుపు_Gen VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // Generate construct
బహిరంగ:
    Ast_కణుపు_Gen(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు{t, fl} {}
    ASTGEN_MEMBERS_Ast_కణుపు_Gen;
};
class Ast_కణుపు_Module VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    స్థిరరాశి string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    పూర్ణాంకం m_లోతు = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    పూర్ణాంకం m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
    VLifetime m_lifetime;  // Lifetime
    VTimescale m_timeunit;  // Global time unit
    VOptionBool m_unconnectedDrive;  // State of `unconnected_drive
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

    bool m_mod_ప్రజా : 1;  // Module has public references
    bool m_modTrace : 1;  // Tracing this module
    bool m_లోపల_Library : 1;  // From a library, no error if not used, never top level
    bool m_ctorVarReset : 1;  // Ctor needs to call ctor_var_reset
    bool m_dead : 1;  // LinkDot believes is dead; will remove in Dead visitors
    bool m_hasGParam : 1;  // Has global parameter (for link)
    bool m_hasParameterList : 1;  // Has #() for parameter declaration
    bool m_hier_ఖండం : 1;  // Hierarchical Block marked by HIER_BLOCK pragma
    bool m_hierParams : 1;  // Block containing params for parameterized hier blocks
    bool m_recursive : 1;  // Recursive module
    bool m_recursiveClone : 1;  // If recursive, what module it clones, otherwise nullptr
    bool m_parameterizedTemplate : 1;  // True when at least one specialized clone exists;
                                       // set by V3Param::deepCloneModule. Suppresses
                                       // width/type errors on the unresolved template.
    bool m_verilatorLib : 1;  // Module is a stub for a Verilator produced --lib-create
రక్షిత:
    Ast_కణుపు_Module(VNType t, File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : Ast_కణుపు{t, fl}
        , m_name{name}
        , m_origName{name}
        , m_libname{libname}
        , m_mod_ప్రజా{false}
        , m_modTrace{false}
        , m_లోపల_Library{false}
        , m_ctorVarReset{false}
        , m_dead{false}
        , m_hasGParam{false}
        , m_hasParameterList{false}
        , m_hier_ఖండం{false}
        , m_hierParams{false}
        , m_recursive{false}
        , m_recursiveClone{false}
        , m_parameterizedTemplate{false}
        , m_verilatorLib{false} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Module;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    మాయా bool timescaleMatters() స్థిరరాశి = 0;
    అంతఃవరుస bool isDollarUnit() స్థిరరాశి;  // Is the $unit package
    అంతఃవరుస bool isConstPool() స్థిరరాశి;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(స్థిరరాశి string& outer) స్థిరరాశి;
    // ACCESSORS
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string origName() స్థిరరాశి override { తిరిగి m_origName; }
    string someInstanceName() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_someInstanceName; }
    శూన్య someInstanceName(స్థిరరాశి string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() స్థిరరాశి { తిరిగి m_defaultDisablep; }
    శూన్య defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool లోపల_Library() స్థిరరాశి { తిరిగి m_లోపల_Library; }
    శూన్య లోపల_Library(bool flag) { m_లోపల_Library = flag; }
    శూన్య లోతు(పూర్ణాంకం విలువ) { m_లోతు = విలువ; }
    పూర్ణాంకం లోతు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_లోతు; }
    శూన్య level(పూర్ణాంకం విలువ) { m_level = విలువ; }
    పూర్ణాంకం level() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_level; }
    string libname() స్థిరరాశి { తిరిగి m_libname; }
    bool is_శిఖరం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి level() == 1; }
    bool mod_ప్రజా() స్థిరరాశి { తిరిగి m_mod_ప్రజా; }
    శూన్య mod_ప్రజా(bool flag) { m_mod_ప్రజా = flag; }
    bool modTrace() స్థిరరాశి { తిరిగి m_modTrace; }
    శూన్య modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() స్థిరరాశి { తిరిగి m_ctorVarReset; }
    శూన్య ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() స్థిరరాశి { తిరిగి m_dead; }
    శూన్య dead(bool flag) { m_dead = flag; }
    bool hasGParam() స్థిరరాశి { తిరిగి m_hasGParam; }
    శూన్య hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() స్థిరరాశి { తిరిగి m_hasParameterList; }
    శూన్య hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hier_ఖండం() స్థిరరాశి { తిరిగి m_hier_ఖండం; }
    శూన్య hier_ఖండం(bool flag) { m_hier_ఖండం = flag; }
    bool hierParams() స్థిరరాశి { తిరిగి m_hierParams; }
    శూన్య hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() స్థిరరాశి { తిరిగి m_recursive; }
    శూన్య recursive(bool flag) { m_recursive = flag; }
    శూన్య recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() స్థిరరాశి { తిరిగి m_recursiveClone; }
    bool parameterizedTemplate() స్థిరరాశి { తిరిగి m_parameterizedTemplate; }
    శూన్య parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    శూన్య verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() స్థిరరాశి { తిరిగి m_verilatorLib; }
    VLifetime lifetime() స్థిరరాశి { తిరిగి m_lifetime; }
    శూన్య lifetime(స్థిరరాశి VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() స్థిరరాశి { తిరిగి m_unconnectedDrive; }
    శూన్య unconnectedDrive(స్థిరరాశి VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_కణుపు_Procedure VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
రక్షిత:
    Ast_కణుపు_Procedure(VNType t, File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : Ast_కణుపు{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        కూడు_Stmtsp(stmtsp);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Procedure;
    // METHODS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isJustOneBodyStmt() స్థిరరాశి { తిరిగి stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() స్థిరరాశి { తిరిగి m_suspendable; }
    శూన్య సమితి_Suspendable() { m_suspendable = true; }
    bool needProcess() స్థిరరాశి { తిరిగి m_needProcess; }
    శూన్య సమితి_Need_Process() { m_needProcess = true; }
};
class Ast_కణుపు_Range VL_NOT_FINAL : బహిరంగ Ast_కణుపు {
    // A range, sized or unsized
రక్షిత:
    Ast_కణుపు_Range(VNType t, File_పంక్తి* fl)
        : Ast_కణుపు{t, fl} {}

బహిరంగ:
    ASTGEN_MEMBERS_Ast_కణుపు_Range;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_చురుకైన final : బహిరంగ Ast_కణుపు {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

బహిరంగ:
    Ast_చురుకైన(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_Sen_వృక్షం* sentreep)
        : ASTGEN_SUPER_చురుకైన(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_Ast_చురుకైన;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    // Statements are broken into pieces, as some must come before others.
    శూన్య sentreep(Ast_Sen_వృక్షం* nodep) { m_sentreep = nodep; }
    Ast_Sen_వృక్షం* sentreep() స్థిరరాశి { తిరిగి m_sentreep; }
    // METHODS
    అంతఃవరుస bool hasClocked() స్థిరరాశి;
    అంతఃవరుస bool hasCombo() స్థిరరాశి;
    అంతఃవరుస bool hasInitial() స్థిరరాశి;
    అంతఃవరుస bool hasStatic() స్థిరరాశి;
};
class AstAlias final : బహిరంగ Ast_కణుపు {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
బహిరంగ:
    AstAlias(File_పంక్తి* fl, Ast_కణుపు_సమాసం* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : బహిరంగ Ast_కణుపు {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
బహిరంగ:
    AstAliasScope(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : బహిరంగ Ast_కణుపు {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
బహిరంగ:
    AstBind(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        కూడు_Cellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Bind Target name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
};
class AstCFunc final : బహిరంగ Ast_కణుపు {
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
    bool m_decl_వ్యక్తిగత : 1;  // Declare it private
    bool m_keep_If_ఖాళీలు : 1;  // Keep declaration and definition separate, even if empty
    bool m_slow : 1;  // Slow routine, called once or just at init time
    bool m_func_ప్రజా : 1;  // From user public task/function
    bool m_isConstructor : 1;  // Is C class constructor
    bool m_isDestructor : 1;  // Is C class destructor
    bool m_is_పద్ధతి : 1;  // Is inside a class definition
    bool m_isLoose : 1;  // Semantically this is a method, but is implemented as a function with
                         // an explicitly passed 'self' pointer as the first argument.  This can
                         // be slightly faster due to __restrict, and we do not declare in header
                         // so adding/removing loose functions doesn't recompile everything.
    bool m_isVirtual : 1;  // Virtual function
    bool m_నమోదు_బిందువు : 1;  // User may call into this top level function
    bool m_dpi_శుద్ధ : 1;  // Pure DPI function
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
    పూర్ణాంకం m_ఖర్చు;  // Function call cost
బహిరంగ:
    AstCFunc(File_పంక్తి* fl, స్థిరరాశి string& name, AstScope* scopep, స్థిరరాశి string& rtnType = "")
        : ASTGEN_SUPER_CFunc(fl) {
        m_isConst = VBoolOrUnknown::BU_UNKNOWN;  // Unknown until analyzed
        m_scopep = scopep;
        m_name = name;
        m_rtnType = rtnType;
        m_isStatic = false;
        m_isTrace = false;
        m_dontCombine = false;
        m_decl_వ్యక్తిగత = false;
        m_keep_If_ఖాళీలు = false;
        m_slow = false;
        m_func_ప్రజా = false;
        m_isConstructor = false;
        m_isDestructor = false;
        m_is_పద్ధతి = true;
        m_isLoose = false;
        m_isVirtual = false;
        m_needProcess = false;
        m_నమోదు_బిందువు = false;
        m_dpi_శుద్ధ = false;
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
        m_ఖర్చు = v3Global.opt.instr_లెక్క_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstCFunc* స్థిరరాశి asamep = VN_DBG_AS(samep, CFunc);
        తిరిగి ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి m_ఖర్చు; }
    VBoolOrUnknown isConst() స్థిరరాశి { తిరిగి m_isConst; }
    శూన్య isConst(bool flag) { m_isConst.సమితి_True_Or_False(flag); }
    శూన్య isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() స్థిరరాశి { తిరిగి m_isStatic; }
    శూన్య isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_isTrace; }
    శూన్య isTrace(bool flag) { m_isTrace = flag; }
    శూన్య cname(స్థిరరాశి string& name) { m_cname = name; }
    string cname() స్థిరరాశి { తిరిగి m_cname; }
    AstScope* scopep() స్థిరరాశి { తిరిగి m_scopep; }
    శూన్య scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() స్థిరరాశి { తిరిగి ((m_rtnType == "") ? "void" : m_rtnType); }
    శూన్య rtnType(స్థిరరాశి string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() స్థిరరాశి { తిరిగి m_dontCombine || isTrace() || నమోదు_బిందువు(); }
    శూన్య dontCombine(bool flag) { m_dontCombine = flag; }
    bool decl_వ్యక్తిగత() స్థిరరాశి { తిరిగి m_decl_వ్యక్తిగత; }
    శూన్య decl_వ్యక్తిగత(bool flag) { m_decl_వ్యక్తిగత = flag; }
    bool keep_If_ఖాళీలు() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_keep_If_ఖాళీలు; }
    శూన్య keep_If_ఖాళీలు(bool flag) { m_keep_If_ఖాళీలు = flag; }
    bool slow() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_slow; }
    శూన్య slow(bool flag) { m_slow = flag; }
    bool func_ప్రజా() స్థిరరాశి { తిరిగి m_func_ప్రజా; }
    శూన్య func_ప్రజా(bool flag) { m_func_ప్రజా = flag; }
    శూన్య argTypes(స్థిరరాశి string& str) { m_argTypes = str; }
    string argTypes() స్థిరరాశి { తిరిగి m_argTypes; }
    శూన్య ifdef(స్థిరరాశి string& str) { m_ifdef = str; }
    string ifdef() స్థిరరాశి { తిరిగి m_ifdef; }
    bool isConstructor() స్థిరరాశి { తిరిగి m_isConstructor; }
    శూన్య isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() స్థిరరాశి { తిరిగి m_isDestructor; }
    శూన్య isDestructor(bool flag) { m_isDestructor = flag; }
    bool is_పద్ధతి() స్థిరరాశి { తిరిగి m_is_పద్ధతి; }
    శూన్య is_పద్ధతి(bool flag) { m_is_పద్ధతి = flag; }
    bool isLoose() స్థిరరాశి { తిరిగి m_isLoose; }
    శూన్య isLoose(bool flag) { m_isLoose = flag; }
    bool is_Proper_పద్ధతి() స్థిరరాశి { తిరిగి is_పద్ధతి() && !isLoose(); }
    bool isVirtual() స్థిరరాశి { తిరిగి m_isVirtual; }
    శూన్య isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() స్థిరరాశి { తిరిగి m_needProcess; }
    శూన్య సమితి_Need_Process() { m_needProcess = true; }
    bool నమోదు_బిందువు() స్థిరరాశి { తిరిగి m_నమోదు_బిందువు; }
    శూన్య నమోదు_బిందువు(bool flag) { m_నమోదు_బిందువు = flag; }
    bool dpi_శుద్ధ() స్థిరరాశి { తిరిగి m_dpi_శుద్ధ; }
    శూన్య dpi_శుద్ధ(bool flag) { m_dpi_శుద్ధ = flag; }
    bool dpiContext() స్థిరరాశి { తిరిగి m_dpiContext; }
    శూన్య dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_dpiExportDispatcher; }
    శూన్య dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() స్థిరరాశి { తిరిగి m_dpiExportImpl; }
    శూన్య dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_dpiImportPrototype; }
    శూన్య dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() స్థిరరాశి { తిరిగి m_dpiImportWrapper; }
    శూన్య dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() స్థిరరాశి { తిరిగి !m_dpiCDecl.ఖాళీలు(); }
    స్థిరరాశి string& dpiCDecl() స్థిరరాశి { తిరిగి m_dpiCDecl; }
    శూన్య dpiCDecl(స్థిరరాశి string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() స్థిరరాశి { తిరిగి m_rtnType == "VlCoroutine"; }
    శూన్య recursive(bool flag) { m_recursive = flag; }
    bool recursive() స్థిరరాశి { తిరిగి m_recursive; }
    శూన్య unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() స్థిరరాశి override { తిరిగి m_unlikely; }  // Note virtual override
    శూన్య noLife(bool flag) { m_noLife = flag; }
    bool noLife() స్థిరరాశి { తిరిగి m_noLife; }
    bool isCovergroupSample() స్థిరరాశి { తిరిగి m_isCovergroupSample; }
    శూన్య isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() స్థిరరాశి { తిరిగి m_voidSelf; }
    శూన్య voidSelf(bool flag) { m_voidSelf = flag; }
    శూన్య ఖర్చు(పూర్ణాంకం ఖర్చు) { m_ఖర్చు = ఖర్చు; }
    // Special methods
    bool ఖాళీలు_Body() స్థిరరాశి {
        తిరిగి !keep_If_ఖాళీలు() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : బహిరంగ Ast_కణుపు {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    స్థిరరాశి string m_name;
    స్థిరరాశి VUseType m_useType;  // What sort of use this is

బహిరంగ:
    AstCUse(File_పంక్తి* fl, VUseType useType, స్థిరరాశి string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    VUseType useType() స్థిరరాశి { తిరిగి m_useType; }
};
class AstCell final : బహిరంగ Ast_కణుపు {
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
    File_పంక్తి* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    పూర్ణాంకం m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
బహిరంగ:
    AstCell(File_పంక్తి* fl, File_పంక్తి* mfl, స్థిరరాశి string& instName, స్థిరరాశి string& modName,
            AstPin* pinsp, AstPin* paramsp, AstRange* rangep)
        : ASTGEN_SUPER_Cell(fl)
        , m_modNameFileline{mfl}
        , m_name{instName}
        , m_origName{instName}
        , m_modName{modName}
        , m_hasIfaceVar{false}
        , m_recursive{false}
        , m_trace{true} {
        కూడు_Pinsp(pinsp);
        కూడు_Paramsp(paramsp);
        కూడు_Rangep(rangep);
    }
    ASTGEN_MEMBERS_AstCell;
    // No cloneRelink, we presume cloneee's want the same module linkages
    శూన్య cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    // ACCESSORS
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Cell name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string origName() స్థిరరాశి override { తిరిగి m_origName; }  // * = Original name
    శూన్య origName(స్థిరరాశి string& name) { m_origName = name; }
    string modName() స్థిరరాశి { తిరిగి m_modName; }  // * = Instance name
    శూన్య modName(స్థిరరాశి string& name) { m_modName = name; }
    File_పంక్తి* modNameFileline() స్థిరరాశి { తిరిగి m_modNameFileline; }
    Ast_కణుపు_Module* modp() స్థిరరాశి { తిరిగి m_modp; }  // [AfterLink] = Pointer to module instantiated
    శూన్య modp(Ast_కణుపు_Module* nodep) { m_modp = nodep; }
    పూర్ణాంకం arrayIdx() స్థిరరాశి { తిరిగి m_arrayIdx; }
    శూన్య arrayIdx(పూర్ణాంకం idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() స్థిరరాశి { తిరిగి m_hasIfaceVar; }
    శూన్య hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    శూన్య trace(bool flag) { m_trace = flag; }
    bool isTrace() స్థిరరాశి { తిరిగి m_trace; }
    శూన్య recursive(bool flag) { m_recursive = flag; }
    bool recursive() స్థిరరాశి { తిరిగి m_recursive; }
};
class AstCellInline final : బహిరంగ Ast_కణుపు {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    స్థిరరాశి std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    స్థిరరాశి std::string m_origModName;  // __BEGIN__ or __FORK__
బహిరంగ:
    AstCellInline(File_పంక్తి* fl, స్థిరరాశి std::string& name, స్థిరరాశి std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    // ACCESSORS
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    std::string origModName() స్థిరరాశి { తిరిగి m_origModName; }
};
class AstCellInlineScope final : బహిరంగ Ast_కణుపు {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    స్థిరరాశి std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
బహిరంగ:
    AstCellInlineScope(File_పంక్తి* fl, స్థిరరాశి std::string& name, స్థిరరాశి std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    // ACCESSORS
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి std::string& name) override { m_name = name; }
    std::string origModName() స్థిరరాశి { తిరిగి m_origModName; }
};
class Ast_Cg_Option_కేటాయించు final : బహిరంగ Ast_కణుపు {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    స్థిరరాశి VCoverOptionType m_optType;  // Option type
    స్థిరరాశి string m_name;  // Original option name (for diagnostics on unknown options)
    స్థిరరాశి bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
బహిరంగ:
    Ast_Cg_Option_కేటాయించు(File_పంక్తి* fl, bool typeOption, VCoverOptionType optType,
                      స్థిరరాశి string& rawName, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cg_Option_కేటాయించు(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Cg_Option_కేటాయించు;
    // ACCESSORS
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    VCoverOptionType optType() స్థిరరాశి { తిరిగి m_optType; }
    bool typeOption() స్థిరరాశి { తిరిగి m_typeOption; }
};
class AstClassExtends final : బహిరంగ Ast_కణుపు {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    స్థిరరాశి bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

బహిరంగ:
    AstClassExtends(File_పంక్తి* fl, Ast_కణుపు* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    string verilogKwd() స్థిరరాశి override { తిరిగి isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() స్థిరరాశి;
    AstClass* classp() స్థిరరాశి;  // Like above, but throws error if nulll
    bool isImplements() స్థిరరాశి { తిరిగి m_isImplements; }
    శూన్య parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() స్థిరరాశి { తిరిగి m_parameterized; }
};
class AstClocking final : బహిరంగ Ast_కణుపు {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    స్థిరరాశి bool m_isGlobal;  // True if global clocking

బహిరంగ:
    AstClocking(File_పంక్తి* fl, స్థిరరాశి std::string& name, Ast_Sen_వృక్షం* sentreep, Ast_కణుపు* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool isDefault() స్థిరరాశి { తిరిగి m_isDefault; }
    bool isGlobal() స్థిరరాశి { తిరిగి m_isGlobal; }
    AstVar* ensureEventp(bool బిడ్డ_DType = false);
    శూన్య makeDefault() { m_isDefault = true; }
};
class Ast_Clocking_అంశం final : బహిరంగ Ast_కణుపు {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

బహిరంగ:
    Ast_Clocking_అంశం(File_పంక్తి* fl, VDirection direction, Ast_కణుపు_సమాసం* skewp, Ast_కణుపు* clockingDeclp)
        : ASTGEN_SUPER_Clocking_అంశం(fl) {
        m_direction = direction;
        this->skewp(skewp);
        ఒకవేళ (Ast_కేటాయించు* స్థిరరాశి clkAssignp = VN_CAST(clockingDeclp, కేటాయించు)) {
            this->assignp(clkAssignp);
        } లేకపోతే {
            exprp(VN_AS(clockingDeclp, కణుపు_సమాసం));
        }
    }
    ASTGEN_MEMBERS_Ast_Clocking_అంశం;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VDirection direction() స్థిరరాశి { తిరిగి m_direction; }
    Ast_Clocking_అంశం* outputp() స్థిరరాశి { తిరిగి m_outputp; }
    శూన్య outputp(Ast_Clocking_అంశం* outputp) { m_outputp = outputp; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
};
class AstConfig final : బహిరంగ Ast_కణుపు {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

బహిరంగ:
    AstConfig(File_పంక్తి* fl, స్థిరరాశి std::string& libname, స్థిరరాశి std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_libname + "." + m_configname; }
    std::string libname() స్థిరరాశి VL_MT_STABLE { తిరిగి m_libname; }
    std::string configname() స్థిరరాశి VL_MT_STABLE { తిరిగి m_configname; }
};
class AstConfigCell final : బహిరంగ Ast_కణుపు {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

బహిరంగ:
    AstConfigCell(File_పంక్తి* fl, స్థిరరాశి std::string& libname, స్థిరరాశి std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_libname + "." + m_cellname; }
    std::string libname() స్థిరరాశి VL_MT_STABLE { తిరిగి m_libname; }
    std::string cellname() స్థిరరాశి VL_MT_STABLE { తిరిగి m_cellname; }
};
class Ast_Config_నియమం final : బహిరంగ Ast_కణుపు {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    స్థిరరాశి bool m_isCell;  // Declared as "cell" versus "instance"

బహిరంగ:
    Ast_Config_నియమం(File_పంక్తి* fl, Ast_కణుపు* cellp, Ast_కణుపు* usep, bool isCell)
        : ASTGEN_SUPER_Config_నియమం(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        కూడు_Usep(usep);
    }
    ASTGEN_MEMBERS_Ast_Config_నియమం;
    bool isCell() స్థిరరాశి VL_MT_STABLE { తిరిగి m_isCell; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class AstConfigUse final : బహిరంగ Ast_కణుపు {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    స్థిరరాశి bool m_isConfig;  // ":config"; Config, not module/primitive name

బహిరంగ:
    AstConfigUse(File_పంక్తి* fl, స్థిరరాశి std::string& libname, స్థిరరాశి std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        కూడు_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_libname + "." + m_cellname; }
    std::string libname() స్థిరరాశి VL_MT_STABLE { తిరిగి m_libname; }
    std::string cellname() స్థిరరాశి VL_MT_STABLE { తిరిగి m_cellname; }
    bool isConfig() స్థిరరాశి VL_MT_STABLE { తిరిగి m_isConfig; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class AstConstraint final : బహిరంగ Ast_కణుపు {
    // Constraint
    // @astgen op1 := itemsp : List[AstNode]
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    string m_name;  // Name of constraint
    VBaseOverride m_పునాది_Override;  // BaseOverride (inital/final/extends)
    bool m_isExternDef = false;  // Extern prototype definition
    bool m_isExternExplicit = false;  // Explicit prototype declaration (has extern)
    bool m_isExternProto = false;  // Prototype declaration (implicit or explicit)
    bool m_is_Kwd_శుద్ధ = false;  // Pure constraint
    bool m_isStatic = false;  // Static constraint
బహిరంగ:
    AstConstraint(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Scope name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    శూన్య పునాది_Override(స్థిరరాశి VBaseOverride& flag) { m_పునాది_Override = flag; }
    VBaseOverride పునాది_Override() స్థిరరాశి { తిరిగి m_పునాది_Override; }
    bool isExternDef() స్థిరరాశి { తిరిగి m_isExternDef; }
    శూన్య isExternDef(bool flag) { m_isExternDef = flag; }
    శూన్య isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() స్థిరరాశి { తిరిగి m_isExternExplicit; }
    శూన్య isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() స్థిరరాశి { తిరిగి m_isExternProto; }
    శూన్య is_Kwd_శుద్ధ(bool flag) { m_is_Kwd_శుద్ధ = flag; }
    bool is_Kwd_శుద్ధ() స్థిరరాశి { తిరిగి m_is_Kwd_శుద్ధ; }
    శూన్య isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() స్థిరరాశి { తిరిగి m_isStatic; }
};
class AstCoverBin final : బహిరంగ Ast_కణుపు {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    స్థిరరాశి string m_name;  // Base name of the bin
    స్థిరరాశి VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

బహిరంగ:
    AstCoverBin(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_కణుపు_సమాసం* iffp = nullptr)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{isIllegal ? VCoverBinsType::BINS_ILLEGAL
                               : (isIgnore ? VCoverBinsType::BINS_IGNORE
                                           : (isWildcard ? VCoverBinsType::BINS_WILDCARD
                                                         : VCoverBinsType::BINS_USER))}
        , m_isWildcard{isWildcard} {
        కూడు_Rangesp(rangesp);
        this->iffp(iffp);
    }
    // Constructor for automatic bins
    AstCoverBin(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_పంక్తి* fl, స్థిరరాశి string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_Cover_Trans_సమితి* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        కూడు_Transp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    VCoverBinsType binsType() స్థిరరాశి { తిరిగి m_binsType; }
    bool isWildcard() స్థిరరాశి { తిరిగి m_isWildcard; }
    bool isArray() స్థిరరాశి { తిరిగి m_isArray; }
    శూన్య isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : బహిరంగ Ast_కణుపు {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    స్థిరరాశి bool m_isNegated;  // Complement the selection within the cross product

బహిరంగ:
    AstCoverBinsof(File_పంక్తి* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_కణుపు* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        కూడు_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    bool isNegated() స్థిరరాశి { తిరిగి m_isNegated; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_START
        స్థిరరాశి AstCoverBinsof* స్థిరరాశి asamep = VN_DBG_AS(samep, CoverBinsof);
        తిరిగి m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : బహిరంగ Ast_కణుపు {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    స్థిరరాశి string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    స్థిరరాశి VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

బహిరంగ:
    AstCoverCrossBin(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* selectp, Ast_కణుపు_సమాసం* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    string verilogKwd() స్థిరరాశి override;
    VCoverBinsType binsType() స్థిరరాశి { తిరిగి m_binsType; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_START
        స్థిరరాశి AstCoverCrossBin* స్థిరరాశి asamep = VN_DBG_AS(samep, CoverCrossBin);
        తిరిగి m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : బహిరంగ Ast_కణుపు {
    // The complete bin-tuple space of the enclosing cross
    స్థిరరాశి string m_name;  // Referenced cross name

బహిరంగ:
    AstCoverCrossRef(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_START
        తిరిగి m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : బహిరంగ Ast_కణుపు {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    స్థిరరాశి bool m_isOr;  // Union (||), rather than intersection (&&)

బహిరంగ:
    AstCoverCrossSelect(File_పంక్తి* fl, Ast_కణుపు* lhsp, Ast_కణుపు* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool isOr() స్థిరరాశి { తిరిగి m_isOr; }
    string verilogKwd() స్థిరరాశి override { తిరిగి isOr() ? "||" : "&&"; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_START
        తిరిగి m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : బహిరంగ Ast_కణుపు {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
బహిరంగ:
    AstCoverCrossWith(File_పంక్తి* fl, Ast_కణుపు* selectp, Ast_కణుపు_సమాసం* filterp, Ast_కణుపు_సమాసం* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : బహిరంగ Ast_కణుపు {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    స్థిరరాశి VCoverOptionType m_optType;  // Option being assigned
    స్థిరరాశి bool m_typeOption;  // type_option (vs option)

బహిరంగ:
    AstCoverOption(File_పంక్తి* fl, bool typeOption, VCoverOptionType optType, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VCoverOptionType optType() స్థిరరాశి { తిరిగి m_optType; }
    bool typeOption() స్థిరరాశి { తిరిగి m_typeOption; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {  // LCOV_EXCL_START
        స్థిరరాశి AstCoverOption* స్థిరరాశి asamep = VN_DBG_AS(samep, CoverOption);
        తిరిగి m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : బహిరంగ Ast_కణుపు {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
బహిరంగ:
    AstCoverToggle(File_పంక్తి* fl, AstCoverInc* incp, Ast_కణుపు_సమాసం* origp, Ast_కణుపు_సమాసం* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి 3 + INSTR_లెక్క_BRANCH + INSTR_లెక్క_LD; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి true; }
    bool isOutputter() override {
        తిరిగి false;  // Though the AstCoverInc under this is an outputter
    }
};
class Ast_Cover_Trans_అంశం final : బహిరంగ Ast_కణుపు {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    స్థిరరాశి VTransRepType m_repType;

బహిరంగ:
    Ast_Cover_Trans_అంశం(File_పంక్తి* fl, Ast_కణుపు* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_Cover_Trans_అంశం(fl)
        , m_repType{repType} {
        కూడు_Valuesp(valuesp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_అంశం;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class Ast_Cover_Trans_సమితి final : బహిరంగ Ast_కణుపు {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
బహిరంగ:
    Ast_Cover_Trans_సమితి(File_పంక్తి* fl, Ast_Cover_Trans_అంశం* itemsp)
        : ASTGEN_SUPER_Cover_Trans_సమితి(fl) {
        కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_సమితి;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class AstCoverWith final : బహిరంగ Ast_కణుపు {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
బహిరంగ:
    AstCoverWith(File_పంక్తి* fl, Ast_కణుపు* subp, Ast_కణుపు_సమాసం* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        కూడు_Subp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : బహిరంగ Ast_కణుపు {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

బహిరంగ:
    AstCovergroup(File_పంక్తి* fl, స్థిరరాశి string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_కణుపు* membersp, Ast_Sen_వృక్షం* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        కూడు_Argsp(argsp);
        కూడు_Sample_Argsp(sampleArgsp);
        కూడు_Membersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override { తిరిగి true; }
};
class AstCoverpointRef final : బహిరంగ Ast_కణుపు {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    స్థిరరాశి string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

బహిరంగ:
    AstCoverpointRef(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
};
class AstDefParam final : బహిరంగ Ast_కణుపు {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
బహిరంగ:
    AstDefParam(File_పంక్తి* fl, Ast_కణుపు_సమాసం* pathp, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి true; }
};
class AstDefaultClocking final : బహిరంగ Ast_కణుపు {
    std::string m_name;  // Clocking block name

బహిరంగ:
    AstDefaultClocking(File_పంక్తి* fl, స్థిరరాశి std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
};
class AstDefaultDisable final : బహిరంగ Ast_కణుపు {
    // @astgen op1 := condp : AstNodeExpr

బహిరంగ:
    AstDefaultDisable(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : బహిరంగ Ast_కణుపు {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
బహిరంగ:
    AstDpiExport(File_పంక్తి* fl, స్థిరరాశి string& vname, స్థిరరాశి string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string cname() స్థిరరాశి { తిరిగి m_cname; }
    శూన్య cname(స్థిరరాశి string& cname) { m_cname = cname; }
};
class AstElabDisplay final : బహిరంగ Ast_కణుపు {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

బహిరంగ:
    అంతఃవరుస AstElabDisplay(File_పంక్తి* fl, VDisplayType dispType, Ast_కణుపు_సమాసం* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!fmtp());
        తిరిగి nullptr;
    }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string verilogKwd() స్థిరరాశి override { తిరిగి "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { తిరిగి true; }  // SPECIAL: $display makes output
    bool isUnlikely() స్థిరరాశి override { తిరిగి true; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    పూర్ణాంకం instr_లెక్క() స్థిరరాశి override { తిరిగి INSTR_లెక్క_PLI; }
    VDisplayType displayType() స్థిరరాశి { తిరిగి m_displayType; }
    శూన్య displayType(VDisplayType type) { m_displayType = type; }
};
class Ast_ఖాళీలు final : బహిరంగ Ast_కణుపు {
    // Represents something missing, e.g. a missing argument in FOREACH
బహిరంగ:
    explicit Ast_ఖాళీలు(File_పంక్తి* fl)
        : ASTGEN_SUPER_ఖాళీలు(fl) {}
    ASTGEN_MEMBERS_Ast_ఖాళీలు;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class Ast_Gen_Case_అంశం final : బహిరంగ Ast_కణుపు {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
బహిరంగ:
    Ast_Gen_Case_అంశం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condsp, Ast_కణుపు* itemsp)
        : ASTGEN_SUPER_Gen_Case_అంశం(fl) {
        కూడు_Condsp(condsp);
        కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_Case_అంశం;
    bool isDefault() స్థిరరాశి { తిరిగి condsp() == nullptr; }
};
class AstImplicit final : బహిరంగ Ast_కణుపు {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
బహిరంగ:
    AstImplicit(File_పంక్తి* fl, Ast_కణుపు* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        కూడు_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : బహిరంగ Ast_కణుపు {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_మాతృ_Name;  // Hierarchical path of the declaring module, before inlining
    string m_పునాది_Name;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
బహిరంగ:
    AstIntfRef(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& మాతృ_Name, స్థిరరాశి string& పునాది_Name,
               స్థిరరాశి string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_మాతృ_Name{మాతృ_Name}
        , m_పునాది_Name{పునాది_Name}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    string మాతృ_Name() స్థిరరాశి { తిరిగి m_మాతృ_Name; }
    string పునాది_Name() స్థిరరాశి { తిరిగి m_పునాది_Name; }
    string modportName() స్థిరరాశి { తిరిగి m_modportName; }
};
class AstLibrary final : బహిరంగ Ast_కణుపు {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

బహిరంగ:
    AstLibrary(File_పంక్తి* fl, స్థిరరాశి std::string& name, Ast_కణుపు_సమాసం* filesp, Ast_కణుపు_సమాసం* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        కూడు_Filesp(filesp);
        కూడు_Incdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
};
class AstModport final : బహిరంగ Ast_కణుపు {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
బహిరంగ:
    AstModport(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        కూడు_Varsp(varsp);
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "modport"; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : బహిరంగ Ast_కణుపు {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
బహిరంగ:
    AstModportClockingRef(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    AstClocking* clockingp() స్థిరరాశి VL_MT_STABLE {
        తిరిగి m_clockingp;
    }  // [After Link] Pointer to clocking block
    శూన్య clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : బహిరంగ Ast_కణుపు {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
బహిరంగ:
    AstModportFTaskRef(File_పంక్తి* fl, స్థిరరాశి string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool isImport() స్థిరరాశి { తిరిగి !m_export; }
    bool isExport() స్థిరరాశి { తిరిగి m_export; }
    Ast_కణుపు_FTask* ftaskp() స్థిరరాశి { తిరిగి m_ftaskp; }  // [After Link] Pointer to variable
    శూన్య ftaskp(Ast_కణుపు_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : బహిరంగ Ast_కణుపు {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    స్థిరరాశి VDirection m_direction;  // Direction of the variable (in/out)
బహిరంగ:
    AstModportVarRef(File_పంక్తి* fl, స్థిరరాశి string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    VDirection direction() స్థిరరాశి { తిరిగి m_direction; }
    AstVar* varp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_varp; }  // [After Link] Pointer to variable
    శూన్య varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : బహిరంగ Ast_కణుపు {
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
    std::string m_resolved_శిఖరం_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::సమితి<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<స్థిరరాశి AstConst*, string> m_const_Orig_Param_పేర్లు;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<స్థిరరాశి Ast_కణుపు*, స్థిరరాశి Ast_కణుపు_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::ENUM_చివర> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::ENUM_చివర> m_dumpTriggersFuncps{};

బహిరంగ:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override;
    శూన్య నెట్టు_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.చొప్పించు(varp); }
    స్థిరరాశి std::సమితి<AstVar*>& deferredParamVarps() స్థిరరాశి { తిరిగి m_deferredParamVarps; }
    శూన్య clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    శూన్య deleteContents();
    శూన్య cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి "$root"; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    Ast_కణుపు_Module* శిఖరం_Modulep() స్థిరరాశి VL_MT_STABLE {  // Top module in hierarchy
        తిరిగి modulesp();  // First one in the list, for now
    }
    Ast_Type_పట్టిక* typeTablep() { తిరిగి m_typeTablep; }
    string astConstOrigParamName(స్థిరరాశి AstConst* nodep) స్థిరరాశి;
    శూన్య astConstOrigParamName(స్థిరరాశి AstConst* nodep, స్థిరరాశి string& name);
    శూన్య astConstOrigParamNameErase(స్థిరరాశి AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    స్థిరరాశి Ast_కణుపు_Module* containingModule(స్థిరరాశి Ast_కణుపు* nodep);
    // Forget remembered modules, as the tree has moved.
    శూన్య clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() స్థిరరాశి { తిరిగి m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() స్థిరరాశి { తిరిగి m_dollarUnitPkgp; }
    శూన్య dollarUnitPkgp(AstPackage* స్థిరరాశి packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() స్థిరరాశి { తిరిగి m_dpiExportTriggerp; }
    శూన్య dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() స్థిరరాశి { తిరిగి m_delaySchedulerp; }
    శూన్య delaySchedulerp(AstVar* స్థిరరాశి varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() స్థిరరాశి { తిరిగి m_nbaEventp; }
    శూన్య nbaEventp(AstVarScope* స్థిరరాశి varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() స్థిరరాశి { తిరిగి m_nbaEventTriggerp; }
    శూన్య nbaEventTriggerp(AstVarScope* స్థిరరాశి varScopep) { m_nbaEventTriggerp = varScopep; }
    శూన్య stdPackagep(AstPackage* స్థిరరాశి packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() స్థిరరాశి { తిరిగి m_stdPackagep; }
    శూన్య stdPackageProcessp(AstClass* స్థిరరాశి classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() స్థిరరాశి { తిరిగి m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_పంక్తి*) స్థిరరాశి;
    Ast_శిఖరం_Scope* శిఖరం_Scopep() స్థిరరాశి { తిరిగి m_శిఖరం_Scopep; }
    శూన్య create_శిఖరం_Scope(AstScope* scopep);
    VTimescale timeunit() స్థిరరాశి { తిరిగి m_timeunit; }
    శూన్య timeunit(స్థిరరాశి VTimescale& విలువ) { m_timeunit = విలువ; }
    VTimescale timeprecision() స్థిరరాశి { తిరిగి m_timeprecision; }
    శూన్య సమయం_Init() {
        m_timeunit = v3Global.opt.సమయం_Default_Unit();
        m_timeprecision = v3Global.opt.సమయం_Default_Prec();
    }
    శూన్య timeprecision_విలీనం(File_పంక్తి*, స్థిరరాశి VTimescale& విలువ);
    శూన్య timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() స్థిరరాశి { తిరిగి m_timescaleSpecified; }
    స్థిరరాశి std::string& resolved_శిఖరం_Module_Name() స్థిరరాశి { తిరిగి m_resolved_శిఖరం_Module_Name; }
    శూన్య resolved_శిఖరం_Module_Name(స్థిరరాశి std::string& విలువ) { m_resolved_శిఖరం_Module_Name = విలువ; }
    uint32_t nTraceCodes() స్థిరరాశి { తిరిగి m_nTraceCodes; }
    శూన్య nTraceCodes(uint32_t విలువ) { m_nTraceCodes = విలువ; }
    AstVarScope* stl_మొదటి_Iterationp();
    శూన్య clear_Stl_మొదటి_Iterationp() { m_stl_మొదటి_Iterationp = nullptr; }
    స్థిరరాశి std::string trace_Lib_శిఖరం_Name() స్థిరరాశి {
        స్థిరరాశి std::string& name = resolved_శిఖరం_Module_Name();
        తిరిగి prettyName(name.ఖాళీలు() ? v3Global.rootp()->శిఖరం_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    శూన్య కూడు_Eval_Stats(స్థిరరాశి std::string& phase);
};
class AstPackageExport final : బహిరంగ Ast_కణుపు {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

బహిరంగ:
    AstPackageExport(File_పంక్తి* fl, స్థిరరాశి string& pkgName, స్థిరరాశి string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    string pkgName() స్థిరరాశి VL_MT_STABLE { తిరిగి m_pkgName; }
    string prettyPkgNameQ() స్థిరరాశి { తిరిగి "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() స్థిరరాశి { తిరిగి m_packagep; }
    శూన్య packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class Ast_Package_Export_నక్షత్రం_నక్షత్రం final : బహిరంగ Ast_కణుపు {
    // A package export *::* declaration
బహిరంగ:
    // cppcheck-suppress noExplicitConstructor
    Ast_Package_Export_నక్షత్రం_నక్షత్రం(File_పంక్తి* fl)
        : ASTGEN_SUPER_Package_Export_నక్షత్రం_నక్షత్రం(fl) {}
    ASTGEN_MEMBERS_Ast_Package_Export_నక్షత్రం_నక్షత్రం;
};
class AstPackageImport final : బహిరంగ Ast_కణుపు {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

బహిరంగ:
    AstPackageImport(File_పంక్తి* fl, AstPackage* packagep, స్థిరరాశి string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_పంక్తి* fl, స్థిరరాశి string& pkgName, స్థిరరాశి string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    string pkgName() స్థిరరాశి VL_MT_STABLE { తిరిగి m_pkgName; }
    string prettyPkgNameQ() స్థిరరాశి { తిరిగి "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() స్థిరరాశి { తిరిగి m_packagep; }
    శూన్య packagep(AstPackage* nodep) { m_packagep = nodep; }

అంతరంగ:
    శూన్య pkgNameFrom();
};
class AstPin final : బహిరంగ Ast_కణుపు {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    పూర్ణాంకం m_pin_అంకె;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_param_మార్గం;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
బహిరంగ:
    AstPin(File_పంక్తి* fl, పూర్ణాంకం pin_అంకె, స్థిరరాశి string& name, Ast_కణుపు* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_అంకె{pin_అంకె}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    శూన్య cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Pin name, ""=go by number
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string prettyOperatorName() స్థిరరాశి override;
    bool dot_నక్షత్రం() స్థిరరాశి { తిరిగి name() == ".*"; }  // Fake name for .* connections until linked
    పూర్ణాంకం pin_అంకె() స్థిరరాశి { తిరిగి m_pin_అంకె; }
    AstVar* modVarp() స్థిరరాశి { తిరిగి m_modVarp; }  // [After Link] Pointer to variable
    శూన్య modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() స్థిరరాశి { తిరిగి m_modPTypep; }
    శూన్య modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() స్థిరరాశి { తిరిగి m_param; }
    శూన్య param(bool flag) { m_param = flag; }
    స్థిరరాశి string& param_మార్గం() స్థిరరాశి { తిరిగి m_param_మార్గం; }
    శూన్య param_మార్గం(స్థిరరాశి string& మార్గం) { m_param_మార్గం = మార్గం; }
    bool svDotName() స్థిరరాశి { తిరిగి m_svDotName; }
    శూన్య svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() స్థిరరాశి { తిరిగి m_svImplicit; }
    శూన్య svImplicit(bool flag) { m_svImplicit = flag; }
};
class Ast_పోర్టు final : బహిరంగ Ast_కణుపు {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    స్థిరరాశి పూర్ణాంకం m_pin_అంకె;  // Pin number
    స్థిరరాశి string m_name;  // Name of pin
బహిరంగ:
    Ast_పోర్టు(File_పంక్తి* fl, పూర్ణాంకం pinnum, స్థిరరాశి string& name)
        : ASTGEN_SUPER_పోర్టు(fl)
        , m_pin_అంకె{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_పోర్టు;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Port name
    పూర్ణాంకం pin_అంకె() స్థిరరాశి { తిరిగి m_pin_అంకె; }  // * = Pin number, for order based instantiation
};
class AstPragma final : బహిరంగ Ast_కణుపు {
    స్థిరరాశి VPragmaType m_pragType;  // Type of pragma
    స్థిరరాశి VTimescale m_timescale;  // For TIMEUNIT_SET
బహిరంగ:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_పంక్తి* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_సమితి, fl, "Should use other constructor");
    }
    AstPragma(File_పంక్తి* fl, స్థిరరాశి VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_సమితి}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VPragmaType pragType() స్థిరరాశి { తిరిగి m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() స్థిరరాశి override { తిరిగి false; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() స్థిరరాశి { తిరిగి m_timescale; }
};
class AstPropSpec final : బహిరంగ Ast_కణుపు {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

బహిరంగ:
    AstPropSpec(File_పంక్తి* fl, Ast_Sen_అంశం* sensesp, Ast_కణుపు_సమాసం* disablep, Ast_కణుపు* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత {
        తిరిగి true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() స్థిరరాశి { తిరిగి m_propStrength; }
};
class AstPull final : బహిరంగ Ast_కణుపు {
    // @astgen op1 := lhsp : AstNodeExpr

    స్థిరరాశి bool m_direction;

బహిరంగ:
    AstPull(File_పంక్తి* fl, Ast_కణుపు_సమాసం* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() స్థిరరాశి { తిరిగి m_direction; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class AstScope final : బహిరంగ Ast_కణుపు {
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
బహిరంగ:
    AstScope(File_పంక్తి* fl, Ast_కణుపు_Module* modp, స్థిరరాశి string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!m_modp);
        తిరిగి nullptr;
    }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Scope name
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    string pretty_Name_సందేశం() స్థిరరాశి override;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    string nameDotless() స్థిరరాశి;
    Ast_కణుపు_Module* modp() స్థిరరాశి { తిరిగి m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    Ast_చురుకైన* comboActivep(bool create);
    //
    AstScope* aboveScopep() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_aboveScopep; }
    శూన్య aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() స్థిరరాశి { తిరిగి m_aboveCellp; }
    శూన్య aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_శిఖరం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి aboveScopep() == nullptr; }  // At top of hierarchy
};
class Ast_Sen_అంశం final : బహిరంగ Ast_కణుపు {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_అంచు_Type;  // Edge type
బహిరంగ:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    Ast_Sen_అంశం(File_పంక్తి* fl, VEdgeType అంచు_Type, Ast_కణుపు_సమాసం* senp, Ast_కణుపు_సమాసం* condp = nullptr)
        : ASTGEN_SUPER_Sen_అంశం(fl)
        , m_అంచు_Type{అంచు_Type} {
        this->sensp(senp);
        this->condp(condp);
    }
    Ast_Sen_అంశం(File_పంక్తి* fl, Combo)
        : ASTGEN_SUPER_Sen_అంశం(fl)
        , m_అంచు_Type{VEdgeType::ET_COMBO} {}
    Ast_Sen_అంశం(File_పంక్తి* fl, Static)
        : ASTGEN_SUPER_Sen_అంశం(fl)
        , m_అంచు_Type{VEdgeType::ET_STATIC} {}
    Ast_Sen_అంశం(File_పంక్తి* fl, Initial)
        : ASTGEN_SUPER_Sen_అంశం(fl)
        , m_అంచు_Type{VEdgeType::ET_INITIAL} {}
    Ast_Sen_అంశం(File_పంక్తి* fl, InitialNBA)
        : ASTGEN_SUPER_Sen_అంశం(fl)
        , m_అంచు_Type{VEdgeType::ET_INITIAL_NBA} {}
    Ast_Sen_అంశం(File_పంక్తి* fl, Final)
        : ASTGEN_SUPER_Sen_అంశం(fl)
        , m_అంచు_Type{VEdgeType::ET_FINAL} {}
    Ast_Sen_అంశం(File_పంక్తి* fl, Never)
        : ASTGEN_SUPER_Sen_అంశం(fl)
        , m_అంచు_Type{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_Ast_Sen_అంశం;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి అంచు_Type() == VN_DBG_AS(samep, Sen_అంశం)->అంచు_Type();
    }
    VEdgeType అంచు_Type() స్థిరరాశి { తిరిగి m_అంచు_Type; }
    శూన్య అంచు_Type(VEdgeType type) {
        m_అంచు_Type = type;
        edit_లెక్క_Inc();
    }
    Ast_కణుపు_Var_Ref* varrefp() స్థిరరాశి { తిరిగి VN_CAST(sensp(), కణుపు_Var_Ref); }
    //
    bool isClocked() స్థిరరాశి { తిరిగి అంచు_Type().clockedStmt(); }
    bool is_Combo_Or_నక్షత్రం() స్థిరరాశి {
        తిరిగి అంచు_Type() == VEdgeType::ET_COMBO || అంచు_Type() == VEdgeType::ET_COMBO_నక్షత్రం;
    }
    bool is_Combo_నక్షత్రం() స్థిరరాశి { తిరిగి అంచు_Type() == VEdgeType::ET_COMBO_నక్షత్రం; }
    bool isHybrid() స్థిరరాశి { తిరిగి అంచు_Type() == VEdgeType::ET_HYBRID; }
    bool isStatic() స్థిరరాశి { తిరిగి అంచు_Type() == VEdgeType::ET_STATIC; }
    bool isInitial() స్థిరరాశి { తిరిగి అంచు_Type() == VEdgeType::ET_INITIAL; }
    bool isFinal() స్థిరరాశి { తిరిగి అంచు_Type() == VEdgeType::ET_FINAL; }
    bool isNever() స్థిరరాశి { తిరిగి అంచు_Type() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_వృక్షం final : బహిరంగ Ast_కణుపు {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
బహిరంగ:
    Ast_Sen_వృక్షం(File_పంక్తి* fl, Ast_Sen_అంశం* sensesp)
        : ASTGEN_SUPER_Sen_వృక్షం(fl) {
        కూడు_Sensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_వృక్షం;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    // METHODS
    bool hasClocked() స్థిరరాశి;  // Includes a clocked statement
    bool has_అంచు() స్థిరరాశి;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() స్థిరరాశి;  // Includes a STATIC SenItem
    bool hasInitial() స్థిరరాశి;  // Includes a INITIAL SenItem
    bool hasFinal() స్థిరరాశి;  // Includes a FINAL SenItem
    bool hasCombo() స్థిరరాశి;  // Includes a COMBO SenItem
    bool hasHybrid() స్థిరరాశి;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : బహిరంగ Ast_కణుపు {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

బహిరంగ:
    AstStrengthSpec(File_పంక్తి* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { తిరిగి m_s0; }
    VStrength strength1() { తిరిగి m_s1; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class AstSystemCSection final : బహిరంగ Ast_కణుపు {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    స్థిరరాశి VSystemCSectionType m_sectionType;  // The section type
    std::string m_పాఠ్యం;  // The text content

బహిరంగ:
    AstSystemCSection(File_పంక్తి* fl, VSystemCSectionType sectionType, స్థిరరాశి std::string& పాఠ్యం)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_పాఠ్యం{పాఠ్యం} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() స్థిరరాశి { తిరిగి m_sectionType; }
    స్థిరరాశి std::string& పాఠ్యం() స్థిరరాశి { తిరిగి m_పాఠ్యం; }
    శూన్య పాఠ్యం(స్థిరరాశి std::string& విలువ) { m_పాఠ్యం = విలువ; }
    శూన్య dump(std::ostream&) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream&) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు*) స్థిరరాశి override { తిరిగి false; }
    bool is_శుద్ధ() override { తిరిగి false; }
    bool isOutputter() override { తిరిగి true; }
};
class Ast_పాఠ్యం final : బహిరంగ Ast_కణుపు {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    స్థిరరాశి std::string m_పాఠ్యం;  // The text to emit
బహిరంగ:
    Ast_పాఠ్యం(File_పంక్తి* fl, స్థిరరాశి std::string& పాఠ్యం)
        : ASTGEN_SUPER_పాఠ్యం(fl)
        , m_పాఠ్యం{పాఠ్యం} {}
    ASTGEN_MEMBERS_Ast_పాఠ్యం;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి పాఠ్యం() == VN_DBG_AS(samep, పాఠ్యం_2)->పాఠ్యం();
    }
    స్థిరరాశి std::string& పాఠ్యం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_పాఠ్యం; }
};
class Ast_పాఠ్యం_ఖండం final : బహిరంగ Ast_కణుపు {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    స్థిరరాశి std::string m_ఉపసర్గ;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    స్థిరరాశి std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    స్థిరరాశి std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
బహిరంగ:
    explicit Ast_పాఠ్యం_ఖండం(File_పంక్తి* fl,  //
                          స్థిరరాశి std::string& ఉపసర్గ = "",  //
                          స్థిరరాశి std::string& separator = "",  //
                          స్థిరరాశి std::string& suffix = "")
        : ASTGEN_SUPER_పాఠ్యం_ఖండం(fl)
        , m_ఉపసర్గ{ఉపసర్గ}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_Ast_పాఠ్యం_ఖండం;
    స్థిరరాశి std::string& ఉపసర్గ() స్థిరరాశి { తిరిగి m_ఉపసర్గ; }
    స్థిరరాశి std::string& separator() స్థిరరాశి { తిరిగి m_separator; }
    స్థిరరాశి std::string& suffix() స్థిరరాశి { తిరిగి m_suffix; }
    // Add some text, or a node to this block
    శూన్య కూడు(స్థిరరాశి string& పాఠ్యం) { కూడు_Nodesp(new Ast_పాఠ్యం{fileline(), పాఠ్యం}); }
    శూన్య కూడు(Ast_కణుపు* nodep) { కూడు_Nodesp(nodep); }
};
class Ast_శిఖరం_Scope final : బహిరంగ Ast_కణుపు {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    మిత్ర class AstNetlist;  // Only the AstNetlist can create one
    Ast_శిఖరం_Scope(File_పంక్తి* fl, AstScope* ascopep)
        : ASTGEN_SUPER_శిఖరం_Scope(fl) {
        this->scopep(ascopep);
    }

బహిరంగ:
    ASTGEN_MEMBERS_Ast_శిఖరం_Scope;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_తాత్కాలిక(స్థిరరాశి string& name, గుర్తులేని వెడల్పు);
    AstVarScope* create_తాత్కాలిక(స్థిరరాశి string& name, Ast_కణుపు_DType* dtypep);
};
class Ast_Type_పట్టిక final : బహిరంగ Ast_కణుపు {
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
    using DetailedMap = std::map<VBasic_Type_కీలకం, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

బహిరంగ:
    explicit Ast_Type_పట్టిక(File_పంక్తి* fl);
    ASTGEN_MEMBERS_Ast_Type_పట్టిక;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* కనుగొను_Basic_DType(File_పంక్తి* fl, VBasicDTypeKwd kwd);
    AstBasicDType* కనుగొను_Logic_బిట్టు_DType(File_పంక్తి* fl, VBasicDTypeKwd kwd, పూర్ణాంకం వెడల్పు, పూర్ణాంకం వెడల్పు_Min,
                                     VSigning numeric);
    AstBasicDType* కనుగొను_Logic_బిట్టు_DType(File_పంక్తి* fl, VBasicDTypeKwd kwd, స్థిరరాశి VNumRange& range,
                                     పూర్ణాంకం వెడల్పు_Min, VSigning numeric);
    AstBasicDType* కనుగొను_Create_Same_DType(AstBasicDType& కణుపు);
    AstBasicDType* కనుగొను_చొప్పించు_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* కనుగొను_Constraint_Ref_DType(File_పంక్తి* fl);
    Ast_ఖాళీలు_వరుస_DType* కనుగొను_ఖాళీలు_వరుస_DType(File_పంక్తి* fl);
    Ast_వరుస_DType* కనుగొను_వరుస_సూచీ_DType(File_పంక్తి* fl);
    Ast_ప్రవాహం_DType* కనుగొను_ప్రవాహం_DType(File_పంక్తి* fl);
    AstVoidDType* కనుగొను_Void_DType(File_పంక్తి* fl);
    శూన్య clearCache();
    శూన్య repairCache();
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
};
class AstTypedef final : బహిరంగ Ast_కణుపు {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_ట్యాగ్;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_అంకె;  // Declaration token number
    bool m_attr_ప్రజా = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

బహిరంగ:
    AstTypedef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* attrsp, VFlag_బిడ్డ_DType,
               Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_అంకె{fl->token_అంకె()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        బిడ్డ_DTypep(dtp);  // Only for parser
        కూడు_Attrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_అంకె{fl->token_అంకె()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() స్థిరరాశి;
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    మాయా Ast_కణుపు_DType* subDTypep() స్థిరరాశి VL_MT_STABLE {
        తిరిగి dtypep() ? dtypep() : బిడ్డ_DTypep();
    }
    // METHODS
    uint32_t decl_Token_అంకె() స్థిరరాశి override { తిరిగి m_decl_Token_అంకె; }
    శూన్య decl_Token_అంకె_సమితి_Min(uint32_t token_అంకె) override {
        m_decl_Token_అంకె = std::min(m_decl_Token_అంకె, token_అంకె);
    }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య name(స్థిరరాశి string& flag) override { m_name = flag; }
    bool attr_ప్రజా() స్థిరరాశి { తిరిగి m_attr_ప్రజా; }
    శూన్య attr_ప్రజా(bool flag) { m_attr_ప్రజా = flag; }
    bool isHideLocal() స్థిరరాశి { తిరిగి m_isHideLocal; }
    శూన్య isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() స్థిరరాశి { తిరిగి m_isHideProtected; }
    శూన్య isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() స్థిరరాశి { తిరిగి m_isUnderClass; }
    శూన్య isUnderClass(bool flag) { m_isUnderClass = flag; }
    శూన్య ట్యాగ్(స్థిరరాశి string& పాఠ్యం) override { m_ట్యాగ్ = పాఠ్యం; }
    string ట్యాగ్() స్థిరరాశి override { తిరిగి m_ట్యాగ్; }
};
class AstTypedefFwd final : బహిరంగ Ast_కణుపు {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    స్థిరరాశి VFwdType m_fwdType;  // Forward type for lint check

బహిరంగ:
    AstTypedefFwd(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    VFwdType fwdType() స్థిరరాశి { తిరిగి m_fwdType; }
};
class Ast_Udp_పట్టిక final : బహిరంగ Ast_కణుపు {
    // @astgen op1 := linesp : List[AstUdpTableLine]
బహిరంగ:
    Ast_Udp_పట్టిక(File_పంక్తి* fl, Ast_Udp_పట్టిక_పంక్తి* linesp)
        : ASTGEN_SUPER_Udp_పట్టిక(fl) {
        this->కూడు_Linesp(linesp);
        ఒకవేళ (!v3Global.has_పట్టిక()) v3Global.సమితి_Has_పట్టిక();
    }
    ASTGEN_MEMBERS_Ast_Udp_పట్టిక;
};
class Ast_Udp_పట్టిక_పంక్తి final : బహిరంగ Ast_కణుపు {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
అంతరంగ:
    స్థిరరాశి bool m_udpIsCombo;  // Combinational or sequential UDP

బహిరంగ:
    class UdpCombo {};
    Ast_Udp_పట్టిక_పంక్తి(UdpCombo, File_పంక్తి* fl, Ast_Udp_పట్టిక_పంక్తి_Val* iFieldsp,
                    Ast_Udp_పట్టిక_పంక్తి_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_పట్టిక_పంక్తి(fl)
        , m_udpIsCombo{true} {
        కూడు_IFieldsp(iFieldsp);
        కూడు_OFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_పట్టిక_పంక్తి(UdpSequential, File_పంక్తి* fl, Ast_Udp_పట్టిక_పంక్తి_Val* iFieldsp,
                    Ast_Udp_పట్టిక_పంక్తి_Val* oFieldsp1, Ast_Udp_పట్టిక_పంక్తి_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_పట్టిక_పంక్తి(fl)
        , m_udpIsCombo{false} {
        కూడు_IFieldsp(iFieldsp);
        కూడు_OFieldsp(oFieldsp1);
        కూడు_OFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_పట్టిక_పంక్తి;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం udpIsCombo() స్థిరరాశి { తిరిగి m_udpIsCombo; }
};
class Ast_Udp_పట్టిక_పంక్తి_Val final : బహిరంగ Ast_కణుపు {
    string m_పాఠ్యం;  // Value character

బహిరంగ:
    Ast_Udp_పట్టిక_పంక్తి_Val(File_పంక్తి* fl, స్థిరరాశి string& పాఠ్యం)
        : ASTGEN_SUPER_Udp_పట్టిక_పంక్తి_Val(fl)
        , m_పాఠ్యం{పాఠ్యం} {}
    ASTGEN_MEMBERS_Ast_Udp_పట్టిక_పంక్తి_Val;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_పాఠ్యం; }
    శూన్య name(std::string స్థిరరాశి& పాఠ్యం) override VL_MT_STABLE { m_పాఠ్యం = పాఠ్యం; }
    string పాఠ్యం() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_పాఠ్యం; }
};
class AstVar final : బహిరంగ Ast_కణుపు {
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
    string m_ట్యాగ్;  // Holds the string of the verilator tag -- used in JSON output.
    VVarType m_varType;  // Type of variable
    VDirection m_direction;  // Direction input/output etc
    VDirection m_declDirection;  // Declared direction input/output etc
    VLifetime m_lifetime;  // Lifetime
    VRandAttr m_rand;  // Randomizability of this variable (rand, randc, etc)
    పూర్ణాంకం m_pin_అంకె = 0;  // For JSON, if non-zero the connection pin number
    పూర్ణాంకం m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
    bool m_libInoutEnable : 1;  // Port is the lowered library inout's enable
    bool m_ansi : 1;  // Params or pins declared in the module header, rather than the body
    bool m_declTyped : 1;  // Declared as type (for dedup check)
    bool m_tristate : 1;  // Inout or triwire or trireg
    bool m_primaryIO : 1;  // In/out to top level (or directly assigned from same)
    bool m_primaryClock : 1;  // In/out to top level and is, or combinationally drives a SenItem
    bool m_sc : 1;  // SystemC variable
    bool m_scSensitive : 1;  // SystemC sensitive() needed
    bool m_sig_ప్రజా : 1;  // User C code accesses this signal or is top signal
    bool m_sig_Mod_ప్రజా : 1;  // User C code accesses this signal and module
    bool m_sig_User_Rd_ప్రజా : 1;  // User C code accesses this signal, read only
    bool m_sigUserRWPublic : 1;  // User C code accesses this signal, read-write
    bool m_వాడిన_Param : 1;  // Parameter is referenced (on link; later signals not setup)
    bool m_వాడిన_Loop_Idx : 1;  // Variable subject of for unrolling
    bool m_funcLocal : 1;  // Local variable for a function
    bool m_funcLocalSticky : 1;  // As m_funcLocal but remains set if var is moved to a static
    bool m_funcReturn : 1;  // Return variable for a function
    bool m_attrScBv : 1;  // User force bit vector attribute
    bool m_attrScBigUint : 1;  // User force sc_biguint attribute
    bool m_attrSFormat : 1;  // User sformat attribute
    bool m_attr_విభజించు_Var : 1;  // declared with split_var metacomment
    bool m_attrFsmState : 1;  // declared with fsm_state metacomment
    bool m_attrFsmArcInclCond : 1;  // declared with fsm_arc_include_cond metacomment
    VFsmExpandType::en m_attrFsmStateExpand : 2;  // FSM expansion type for FSM coverage
    bool m_const_Pool_నమోదు : 1;  // Constant pool variable
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
    bool m_is_Iface_మాతృ : 1;  // dtype is reference to interface present in this module
    bool m_is_Iface_Array_విభజించు : 1;  // Interface array that was split into element variables by
                                   // V3Param, until V3Width removes it
    bool m_is_అంతర్గత : 1;  // Internal state, don't add to method pinter
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
    bool m_is_Forced_By_సంకేతం : 1;  // May be forced/released from AstAssignForce/AstRelease
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
    bool m_process_వరుస : 1;  // Process queue variable
    bool m_mtask_Cache_పంక్తి_Align : 1;  // Start MTask affinity group on a cache line
    శూన్య init() {
        m_libInoutEnable = false;
        m_ansi = false;
        m_declTyped = false;
        m_tristate = false;
        m_primaryIO = false;
        m_primaryClock = false;
        m_sc = false;
        m_scSensitive = false;
        m_వాడిన_Param = false;
        m_వాడిన_Loop_Idx = false;
        m_sig_ప్రజా = false;
        m_sig_Mod_ప్రజా = false;
        m_sig_User_Rd_ప్రజా = false;
        m_sigUserRWPublic = false;
        m_funcLocal = false;
        m_funcLocalSticky = false;
        m_funcReturn = false;
        m_attrScBv = false;
        m_attrScBigUint = false;
        m_attrSFormat = false;
        m_attr_విభజించు_Var = false;
        m_attrFsmState = false;
        m_attrFsmArcInclCond = false;
        m_attrFsmStateExpand = VFsmExpandType::DEFAULT;
        m_const_Pool_నమోదు = false;
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
        m_is_Iface_మాతృ = false;
        m_is_Iface_Array_విభజించు = false;
        m_is_అంతర్గత = false;
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
        m_is_Forced_By_సంకేతం = false;
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
        m_process_వరుస = false;
        m_mtask_Cache_పంక్తి_Align = false;
    }

బహిరంగ:
    AstVar(File_పంక్తి* fl, VVarType type, స్థిరరాశి string& name, VFlag_బిడ్డ_DType, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        బిడ్డ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_పంక్తి* fl, VVarType type, స్థిరరాశి string& name, Ast_కణుపు_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_పంక్తి* fl, VVarType type, స్థిరరాశి string& name, VFlagLogicPacked, పూర్ణాంకం wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_సమితి_Logic_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_పంక్తి* fl, VVarType type, స్థిరరాశి string& name, VFlag_బిట్టు_Packed, పూర్ణాంకం wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_సమితి_బిట్టు_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_పంక్తి* fl, VVarType type, స్థిరరాశి string& name, స్థిరరాశి AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ఒకవేళ (examplep->బిడ్డ_DTypep()) బిడ్డ_DTypep(examplep->బిడ్డ_DTypep()->clone_వృక్షం(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    అంతఃవరుస bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }  // * = Var name
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    string origName() స్థిరరాశి override { తిరిగి m_origName; }  // * = Original name
    శూన్య origName(స్థిరరాశి string& name) { m_origName = name; }
    VVarType varType() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_varType; }  // * = Type of variable
    శూన్య direction(స్థిరరాశి VDirection& flag) {
        m_direction = flag;
        ఒకవేళ (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_direction; }
    bool isIO() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_direction != VDirection::NONE; }
    bool isVLIO() స్థిరరాశి {
        స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = basicp();
        తిరిగి isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    శూన్య declDirection(స్థిరరాశి VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() స్థిరరాశి { తిరిగి m_declDirection; }
    పూర్ణాంకం libInoutId() స్థిరరాశి { తిరిగి m_libInoutId; }
    శూన్య libInoutId(పూర్ణాంకం id) { m_libInoutId = id; }
    bool libInoutEnable() స్థిరరాశి { తిరిగి m_libInoutEnable; }
    శూన్య libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    శూన్య varType(VVarType type) { m_varType = type; }
    శూన్య var_Type2_బహిర్గతం() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    శూన్య var_Type2_లోపల() {
        m_tristate = false;
        m_direction = VDirection::ఆదానం;
    }
    string scType() స్థిరరాశి;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) స్థిరరాశి;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) స్థిరరాశి;
    string dpiTmpVarType(స్థిరరాశి string& varName) స్థిరరాశి;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) స్థిరరాశి;
    string vlEnumType() స్థిరరాశి;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) స్థిరరాశి;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(స్థిరరాశి string& propName) స్థిరరాశి;  // Return VerilatorVarProps declaration
    శూన్య combineType(VVarType type);
    Ast_కణుపు_DType* get_బిడ్డ_DTypep() స్థిరరాశి override { తిరిగి బిడ్డ_DTypep(); }
    Ast_కణుపు_DType* dtypeSkipRefp() స్థిరరాశి VL_MT_STABLE { తిరిగి subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() స్థిరరాశి VL_MT_STABLE { తిరిగి subDTypep()->basicp(); }
    మాయా Ast_కణుపు_DType* subDTypep() స్థిరరాశి VL_MT_STABLE {
        తిరిగి dtypep() ? dtypep() : బిడ్డ_DTypep();
    }
    శూన్య ansi(bool flag) { m_ansi = flag; }
    శూన్య declTyped(bool flag) { m_declTyped = flag; }
    శూన్య sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    శూన్య attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    శూన్య attrScBv(bool flag) { m_attrScBv = flag; }
    శూన్య attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    శూన్య attrSFormat(bool flag) { m_attrSFormat = flag; }
    శూన్య attr_విభజించు_Var(bool flag) { m_attr_విభజించు_Var = flag; }
    శూన్య attrFsmState(bool flag) { m_attrFsmState = flag; }
    శూన్య attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    శూన్య attrFsmStateExpand(స్థిరరాశి VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool const_Pool_నమోదు() స్థిరరాశి { తిరిగి m_const_Pool_నమోదు; }
    శూన్య సమితి_Const_Pool_నమోదు() { m_const_Pool_నమోదు = true; }
    bool covergroupRefMember() స్థిరరాశి { తిరిగి m_covergroupRefMember; }
    శూన్య covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() స్థిరరాశి { తిరిగి m_embeddedCovergroup; }
    శూన్య embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    శూన్య rand(స్థిరరాశి VRandAttr flag) { m_rand = flag; }
    శూన్య వాడిన_Param(bool flag) { m_వాడిన_Param = flag; }
    శూన్య వాడిన_Loop_Idx(bool flag) { m_వాడిన_Loop_Idx = flag; }
    శూన్య sig_ప్రజా(bool flag) { m_sig_ప్రజా = flag; }
    శూన్య sig_Mod_ప్రజా(bool flag) { m_sig_Mod_ప్రజా = flag; }
    శూన్య sig_User_Rd_ప్రజా(bool flag) {
        m_sig_User_Rd_ప్రజా = flag;
        ఒకవేళ (flag) sig_ప్రజా(true);
    }
    శూన్య sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        ఒకవేళ (flag) sig_User_Rd_ప్రజా(true);
    }
    శూన్య sc(bool flag) { m_sc = flag; }
    శూన్య scSensitive(bool flag) { m_scSensitive = flag; }
    శూన్య primaryIO(bool flag) { m_primaryIO = flag; }
    శూన్య isConst(bool flag) { m_isConst = flag; }
    శూన్య isContinuously(bool flag) { m_isContinuously = flag; }
    శూన్య is_Iface_మాతృ(bool flag) { m_is_Iface_మాతృ = flag; }
    శూన్య is_Iface_Array_విభజించు(bool flag) { m_is_Iface_Array_విభజించు = flag; }
    శూన్య is_అంతర్గత(bool flag) { m_is_అంతర్గత = flag; }
    శూన్య isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    శూన్య funcLocal(bool flag) {
        m_funcLocal = flag;
        ఒకవేళ (flag) m_funcLocalSticky = true;
    }
    శూన్య funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() స్థిరరాశి { తిరిగి m_gotNansiType; }
    శూన్య gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() స్థిరరాశి { తిరిగి m_hasStrengthAssignment; }
    శూన్య hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() స్థిరరాశి { తిరిగి m_hasUserInit; }
    శూన్య hasUserInit(bool flag) { m_hasUserInit = flag; }
    శూన్య icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() స్థిరరాశి { తిరిగి m_icoMaybeWritten; }
    శూన్య isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() స్థిరరాశి { తిరిగి m_isClockingDriven; }
    bool isDpiOpenArray() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_isDpiOpenArray; }
    శూన్య isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() స్థిరరాశి { తిరిగి m_isHideLocal; }
    శూన్య isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() స్థిరరాశి { తిరిగి m_isHideProtected; }
    శూన్య isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() స్థిరరాశి { తిరిగి m_noCReset; }
    శూన్య noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() స్థిరరాశి { తిరిగి m_noReset; }
    శూన్య noReset(bool flag) { m_noReset = flag; }
    bool noSubst() స్థిరరాశి { తిరిగి m_noSubst; }
    శూన్య noSubst(bool flag) { m_noSubst = flag; }
    bool process_వరుస() స్థిరరాశి { తిరిగి m_process_వరుస; }
    శూన్య process_వరుస(bool flag) { m_process_వరుస = flag; }
    bool sampled() స్థిరరాశి { తిరిగి m_sampled; }
    శూన్య sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() స్థిరరాశి { తిరిగి m_substConstOnly; }
    శూన్య substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() స్థిరరాశి { తిరిగి m_overriddenParam; }
    శూన్య overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() స్థిరరాశి { తిరిగి m_untypedParam; }
    శూన్య untypedParam(bool flag) { m_untypedParam = flag; }
    శూన్య trace(bool flag) { m_trace = flag; }
    శూన్య isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() స్థిరరాశి { తిరిగి m_isForceable; }
    శూన్య సమితి_Forceable() { m_isForceable = true; }
    శూన్య సమితి_Forced_By_సంకేతం() { m_is_Forced_By_సంకేతం = true; }
    bool isForced() స్థిరరాశి { తిరిగి m_isForceable || m_is_Forced_By_సంకేతం; }
    bool isReadByDpi() స్థిరరాశి { తిరిగి m_isReadByDpi; }
    శూన్య సమితి_Read_By_Dpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() స్థిరరాశి { తిరిగి m_isWrittenByDpi; }
    శూన్య సమితి_Written_By_Dpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() స్థిరరాశి { తిరిగి m_isWrittenBySuspendable; }
    శూన్య సమితి_Written_By_Suspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() స్థిరరాశి { తిరిగి m_ignorePostRead; }
    శూన్య సమితి_Ignore_Post_Read() { m_ignorePostRead = true; }
    bool ignorePostWrite() స్థిరరాశి { తిరిగి m_ignorePostWrite; }
    శూన్య సమితి_Ignore_Post_Write() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() స్థిరరాశి { తిరిగి m_ignoreSchedWrite; }
    శూన్య సమితి_Ignore_Sched_Write() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() స్థిరరాశి { తిరిగి m_dfgTriLowered; }
    శూన్య సమితి_Dfg_Tri_Lowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() స్థిరరాశి { తిరిగి m_dfgAllowMultidriveTri; }
    శూన్య సమితి_Dfg_Allow_Multidrive_Tri() { m_dfgAllowMultidriveTri = true; }
    శూన్య globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() స్థిరరాశి { తిరిగి m_globalConstrained; }
    bool isStdRandomizeArg() స్థిరరాశి { తిరిగి m_isStdRandomizeArg; }
    శూన్య సమితి_Std_Randomize_Arg() { m_isStdRandomizeArg = true; }
    // METHODS
    శూన్య name(స్థిరరాశి string& name) override { m_name = name; }
    శూన్య ట్యాగ్(స్థిరరాశి string& పాఠ్యం) override { m_ట్యాగ్ = పాఠ్యం; }
    string ట్యాగ్() స్థిరరాశి override { తిరిగి m_ట్యాగ్; }
    bool isAnsi() స్థిరరాశి { తిరిగి m_ansi; }
    bool isContinuously() స్థిరరాశి { తిరిగి m_isContinuously; }
    bool isDeclTyped() స్థిరరాశి { తిరిగి m_declTyped; }
    bool isInout() స్థిరరాశి { తిరిగి m_direction.isInout(); }
    bool isInoutOrRef() స్థిరరాశి { తిరిగి m_direction.isInoutOrRef(); }
    bool is_ఆదానం() స్థిరరాశి { తిరిగి m_direction.is_ఆదానం(); }
    bool isNonOutput() స్థిరరాశి { తిరిగి m_direction.isNonOutput(); }
    bool isReadOnly() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_direction.isReadOnly(); }
    bool isConstRef() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_direction.isConstRef(); }
    bool isRef() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_direction.isRef(); }
    bool isWritable() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_direction.isWritable(); }
    bool isTristate() స్థిరరాశి { తిరిగి m_tristate; }
    bool isPrimaryIO() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_primaryIO; }
    bool isPrimaryInish() స్థిరరాశి { తిరిగి isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_primaryClock; }
    శూన్య సమితి_Primary_Clock() { m_primaryClock = true; }
    bool isIfaceRef() స్థిరరాశి { తిరిగి varType() == VVarType::IFACEREF; }
    శూన్య సమితి_Iface_Ref() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool is_Iface_మాతృ() స్థిరరాశి { తిరిగి m_is_Iface_మాతృ; }
    bool is_Iface_Array_విభజించు() స్థిరరాశి { తిరిగి m_is_Iface_Array_విభజించు; }
    bool is_అంతర్గత() స్థిరరాశి { తిరిగి m_is_అంతర్గత; }
    bool isIfaceParam() స్థిరరాశి { తిరిగి m_isIfaceParam; }
    bool isSignal() స్థిరరాశి { తిరిగి varType().isSignal(); }
    bool isNet() స్థిరరాశి { తిరిగి varType().isNet(); }
    bool isWor() స్థిరరాశి { తిరిగి varType().isWor(); }
    bool isWiredNet() స్థిరరాశి { తిరిగి varType().isWiredNet(); }
    bool is_తాత్కాలిక() స్థిరరాశి { తిరిగి varType().is_తాత్కాలిక(); }
    bool isClassMember() స్థిరరాశి { తిరిగి varType() == VVarType::MEMBER; }
    bool isVirtIface() స్థిరరాశి {
        ఒకవేళ (AstIfaceRefDType* స్థిరరాశి dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            తిరిగి dtp->isVirtual();
        }
        తిరిగి false;
    }
    bool is_Statement_తాత్కాలిక() స్థిరరాశి { తిరిగి varType() == VVarType::STMTTEMP; }
    bool isXTemp() స్థిరరాశి { తిరిగి varType() == VVarType::XTEMP; }
    bool isParam() స్థిరరాశి { తిరిగి varType().isParam(); }
    bool isGParam() స్థిరరాశి { తిరిగి varType() == VVarType::GPARAM; }
    bool isGenVar() స్థిరరాశి { తిరిగి varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() స్థిరరాశి {
        తిరిగి isIO() || isSignal() || isClassMember() || is_తాత్కాలిక() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() స్థిరరాశి {
        తిరిగి !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool is_బిట్టు_Logic() స్థిరరాశి {
        స్థిరరాశి AstBasicDType* స్థిరరాశి bdtypep = basicp();
        తిరిగి bdtypep && bdtypep->is_బిట్టు_Logic();
    }
    bool is_వాడిన_Param() స్థిరరాశి { తిరిగి m_వాడిన_Param; }
    bool is_వాడిన_Loop_Idx() స్థిరరాశి { తిరిగి m_వాడిన_Loop_Idx; }
    bool mtask_Cache_పంక్తి_Align() స్థిరరాశి { తిరిగి m_mtask_Cache_పంక్తి_Align; }
    శూన్య mtask_Cache_పంక్తి_Align(bool flag) { m_mtask_Cache_పంక్తి_Align = flag; }
    bool isSc() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_sc; }
    bool isScQuad() స్థిరరాశి;
    bool isScBv() స్థిరరాశి VL_MT_STABLE;
    bool isScUint() స్థిరరాశి;
    bool isScUintBool() స్థిరరాశి;
    bool isScBigUint() స్థిరరాశి VL_MT_STABLE;
    bool isScSensitive() స్థిరరాశి { తిరిగి m_scSensitive; }
    bool is_Sig_ప్రజా() స్థిరరాశి;
    bool is_Sig_Mod_ప్రజా() స్థిరరాశి { తిరిగి m_sig_Mod_ప్రజా && !isIfaceRef(); }
    bool is_Sig_User_Rd_ప్రజా() స్థిరరాశి { తిరిగి m_sig_User_Rd_ప్రజా && !isIfaceRef(); }
    bool isSigUserRWPublic() స్థిరరాశి { తిరిగి m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() స్థిరరాశి { తిరిగి m_trace; }
    bool isRand() స్థిరరాశి { తిరిగి m_rand.isRand(); }
    bool isRandC() స్థిరరాశి { తిరిగి m_rand.isRandC(); }
    bool isConst() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_isConst; }
    bool isLatched() స్థిరరాశి { తిరిగి m_isLatched; }
    bool isFuncLocal() స్థిరరాశి { తిరిగి m_funcLocal; }
    bool isFuncLocalSticky() స్థిరరాశి { తిరిగి m_funcLocalSticky; }
    bool isFuncReturn() స్థిరరాశి { తిరిగి m_funcReturn; }
    bool isPullup() స్థిరరాశి { తిరిగి m_isPullup; }
    bool isPulldown() స్థిరరాశి { తిరిగి m_isPulldown; }
    bool attrScBv() స్థిరరాశి { తిరిగి m_attrScBv; }
    bool attrScBigUint() స్థిరరాశి { తిరిగి m_attrScBigUint; }
    bool attrFileDescr() స్థిరరాశి { తిరిగి m_attrFileDescr; }
    bool attrSFormat() స్థిరరాశి { తిరిగి m_attrSFormat; }
    bool attr_విభజించు_Var() స్థిరరాశి { తిరిగి m_attr_విభజించు_Var; }
    bool attrFsmState() స్థిరరాశి { తిరిగి m_attrFsmState; }
    bool attrFsmArcInclCond() స్థిరరాశి { తిరిగి m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() స్థిరరాశి { తిరిగి m_attrFsmStateExpand; }
    AstIface* sensIfacep() స్థిరరాశి { తిరిగి m_sensIfacep; }
    VRandAttr rand() స్థిరరాశి { తిరిగి m_rand; }
    string verilogKwd() స్థిరరాశి override;
    శూన్య lifetime(స్థిరరాశి VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() స్థిరరాశి { తిరిగి m_lifetime; }
    శూన్య pin_అంకె(పూర్ణాంకం id) { m_pin_అంకె = id; }
    పూర్ణాంకం pin_అంకె() స్థిరరాశి { తిరిగి m_pin_అంకె; }
    శూన్య propagateAttrFrom(స్థిరరాశి AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        ఒకవేళ (fromp->attrFileDescr()) attrFileDescr(true);
        ఒకవేళ (fromp->isContinuously()) isContinuously(true);
    }
    శూన్య propagate_విభజించు_Attr_From(స్థిరరాశి AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attr_విభజించు_Var(fromp->attr_విభజించు_Var());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    స్థిరరాశి అక్షరం* cannot_విభజించు_రకం_Reason() స్థిరరాశి;
    శూన్య propagateWrapAttrFrom(స్థిరరాశి AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    శూన్య combineType(స్థిరరాశి AstVar* otherp);
    bool needsCReset() స్థిరరాశి {
        తిరిగి !is_Iface_మాతృ() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_తాత్కాలిక()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    స్థిర AstVar* scVarRecurse(Ast_కణుపు* nodep);
};
class AstVarScope final : బహిరంగ Ast_కణుపు {
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
    bool m_fork_By_విలువ : 1;  // Set before a fork, whose processes get it by value
బహిరంగ:
    AstVarScope(File_పంక్తి* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_విలువ = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    శూన్య cloneRelink() override {
        ఒకవేళ (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి scopep()->name() + "->" + varp()->name(); }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    AstVar* varp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_varp; }  // [After Link] Pointer to variable
    శూన్య varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_scopep; }  // Pointer to scope it's under
    శూన్య scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() స్థిరరాశి { తిరిగి m_trace; }
    శూన్య trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() స్థిరరాశి { తిరిగి m_optimizeLifePost; }
    శూన్య optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_విలువ() స్థిరరాశి { తిరిగి m_fork_By_విలువ; }
    శూన్య fork_By_విలువ(bool flag) { m_fork_By_విలువ = flag; }
};

// === AstNodeConsItem ===
class Ast_Cons_Array_అంశం final : బహిరంగ Ast_కణుపు_Cons_అంశం {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    స్థిరరాశి uint64_t m_సూచీ;  // Index of the element, with the array's lo() at 0
బహిరంగ:
    Ast_Cons_Array_అంశం(File_పంక్తి* fl, uint64_t సూచీ, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cons_Array_అంశం(fl, valuep)
        , m_సూచీ{సూచీ} {}
    ASTGEN_MEMBERS_Ast_Cons_Array_అంశం;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), Cons_Array_అంశం));
        తిరిగి nullptr;
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        తిరిగి m_సూచీ == VN_DBG_AS(samep, Cons_Array_అంశం)->m_సూచీ;
    }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }  // By AstConsArray's map
    uint64_t సూచీ() స్థిరరాశి { తిరిగి m_సూచీ; }
};
class Ast_Cons_Assoc_అంశం final : బహిరంగ Ast_కణుపు_Cons_అంశం {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
బహిరంగ:
    Ast_Cons_Assoc_అంశం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* keyp, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cons_Assoc_అంశం(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Assoc_అంశం;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), Cons_Assoc_అంశం));
        తిరిగి nullptr;
    }
};
class Ast_Cons_Dyn_Array_అంశం final : బహిరంగ Ast_కణుపు_Cons_అంశం {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
బహిరంగ:
    Ast_Cons_Dyn_Array_అంశం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_అంశం(fl, valuep) {}
    Ast_Cons_Dyn_Array_అంశం(File_పంక్తి* fl, AstConst* keyp, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_అంశం(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Dyn_Array_అంశం;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), Cons_Dyn_Array_అంశం));
        తిరిగి nullptr;
    }
};
class Ast_Cons_వరుస_అంశం final : బహిరంగ Ast_కణుపు_Cons_అంశం {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
బహిరంగ:
    Ast_Cons_వరుస_అంశం(File_పంక్తి* fl, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cons_వరుస_అంశం(fl, valuep) {}
    ASTGEN_MEMBERS_Ast_Cons_వరుస_అంశం;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), Cons_వరుస) && !VN_IS(backp(), Cons_వరుస_అంశం));
        తిరిగి nullptr;
    }
};
class Ast_Cons_Struct_అంశం final : బహిరంగ Ast_కణుపు_Cons_అంశం {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
బహిరంగ:
    Ast_Cons_Struct_అంశం(File_పంక్తి* fl, AstMemberDType* memberp, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cons_Struct_అంశం(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_Ast_Cons_Struct_అంశం;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), Cons_Struct_అంశం));
        తిరిగి nullptr;
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    AstMemberDType* memberp() స్థిరరాశి { తిరిగి m_memberp; }
};
class Ast_Cons_Wildcard_అంశం final : బహిరంగ Ast_కణుపు_Cons_అంశం {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
బహిరంగ:
    Ast_Cons_Wildcard_అంశం(File_పంక్తి* fl, AstConst* keyp, Ast_కణుపు_సమాసం* valuep)
        : ASTGEN_SUPER_Cons_Wildcard_అంశం(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Wildcard_అంశం;
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), Cons_Wildcard_అంశం));
        తిరిగి nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : బహిరంగ Ast_కణుపు_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsm_గమ్యం;  // FSM destination state label
    string m_fsm_ట్యాగ్;  // FSM arc kind tag (e.g. reset, reset_include, default)
    పూర్ణాంకం m_స్థానభ్రంశం;  // Offset column numbers to uniq-ify IFs
బహిరంగ:
    AstCoverOtherDecl(File_పంక్తి* fl, స్థిరరాశి string& పుట, స్థిరరాశి string& comment,
                      స్థిరరాశి string& linescov, పూర్ణాంకం స్థానభ్రంశం, స్థిరరాశి string& fsmVar = "",
                      స్థిరరాశి string& fsmFrom = "", స్థిరరాశి string& fsm_గమ్యం = "",
                      స్థిరరాశి string& fsm_ట్యాగ్ = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, పుట, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsm_గమ్యం{fsm_గమ్యం}
        , m_fsm_ట్యాగ్{fsm_ట్యాగ్}
        , m_స్థానభ్రంశం{స్థానభ్రంశం} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం స్థానభ్రంశం() స్థిరరాశి { తిరిగి m_స్థానభ్రంశం; }
    పూర్ణాంకం కొలత() స్థిరరాశి override { తిరిగి 1; }
    స్థిరరాశి string& linescov() స్థిరరాశి { తిరిగి m_linescov; }
    స్థిరరాశి string& fsmVar() స్థిరరాశి { తిరిగి m_fsmVar; }
    స్థిరరాశి string& fsmFrom() స్థిరరాశి { తిరిగి m_fsmFrom; }
    స్థిరరాశి string& fsm_గమ్యం() స్థిరరాశి { తిరిగి m_fsm_గమ్యం; }
    స్థిరరాశి string& fsm_ట్యాగ్() స్థిరరాశి { తిరిగి m_fsm_ట్యాగ్; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstCoverOtherDecl* స్థిరరాశి asamep = VN_DBG_AS(samep, CoverOtherDecl);
        తిరిగి Super::same_కణుపు(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : బహిరంగ Ast_కణుపు_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    స్థిరరాశి VNumRange m_range;  // Packed array range covering each toggle bit
బహిరంగ:
    AstCoverToggleDecl(File_పంక్తి* fl, స్థిరరాశి string& పుట, స్థిరరాశి string& comment,
                       స్థిరరాశి VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, పుట, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    పూర్ణాంకం కొలత() స్థిరరాశి override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        తిరిగి 2 * m_range.elements();
    }
    స్థిరరాశి VNumRange& range() స్థిరరాశి { తిరిగి m_range; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstCoverToggleDecl* స్థిరరాశి asamep = VN_DBG_AS(samep, CoverToggleDecl);
        తిరిగి Super::same_కణుపు(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : బహిరంగ Ast_కణుపు_FTask {
    // A function inside a module
బహిరంగ:
    AstFunc(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* stmtp, Ast_కణుపు* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    Ast_కణుపు_FTask* cloneType(స్థిరరాశి string& name) override {
        తిరిగి new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "function"; }
};
class AstLet final : బహిరంగ Ast_కణుపు_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
బహిరంగ:
    AstLet(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    స్థిరరాశి అక్షరం* broken() స్థిరరాశి override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_సమాసం));
        తిరిగి nullptr;
    }
    Ast_కణుపు_FTask* cloneType(స్థిరరాశి string& name) override { తిరిగి new AstLet{fileline(), name}; }
    string verilogKwd() స్థిరరాశి override { తిరిగి "let"; }
};
class AstProperty final : బహిరంగ Ast_కణుపు_FTask {
    // A property inside a module
బహిరంగ:
    AstProperty(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    Ast_కణుపు_FTask* cloneType(స్థిరరాశి string& name) override {
        తిరిగి new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "property"; }
};
class AstSequence final : బహిరంగ Ast_కణుపు_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
బహిరంగ:
    AstSequence(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool hasDType() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    Ast_కణుపు_FTask* cloneType(స్థిరరాశి string& name) override {
        తిరిగి new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "sequence"; }
    bool isReferenced() స్థిరరాశి { తిరిగి m_referenced; }
    శూన్య isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : బహిరంగ Ast_కణుపు_FTask {
    // A task inside a module
బహిరంగ:
    AstTask(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_కణుపు_FTask* cloneType(స్థిరరాశి string& name) override {
        తిరిగి new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() స్థిరరాశి override { తిరిగి "task"; }
};

// === AstNodeFile ===
class AstCFile final : బహిరంగ Ast_కణుపు_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_ఆధారం : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
బహిరంగ:
    AstCFile(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_ఆధారం{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
    uint64_t complexityScore() స్థిరరాశి { తిరిగి m_complexityScore; }
    శూన్య complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() స్థిరరాశి { తిరిగి m_slow; }
    శూన్య slow(bool flag) { m_slow = flag; }
    bool ఆధారం() స్థిరరాశి { తిరిగి m_ఆధారం; }
    శూన్య ఆధారం(bool flag) { m_ఆధారం = flag; }
    bool support() స్థిరరాశి { తిరిగి m_support; }
    శూన్య support(bool flag) VL_MT_సురక్షిత { m_support = flag; }
};
class AstVFile final : బహిరంగ Ast_కణుపు_File {
    // Verilog output file
    // Parents:  NETLIST
బహిరంగ:
    AstVFile(File_పంక్తి* fl, స్థిరరాశి string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    శూన్య dump(std::ostream& str = std::cout) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str = std::cout) స్థిరరాశి override;
};
class AstCoverCross final : బహిరంగ Ast_కణుపు_Func_Cov_అంశం {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
బహిరంగ:
    AstCoverCross(File_పంక్తి* fl, స్థిరరాశి string& name, AstCoverpointRef* itemsp,
                  Ast_కణుపు_సమాసం* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        కూడు_Itemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class AstCoverpoint final : బహిరంగ Ast_కణుపు_Func_Cov_అంశం {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
బహిరంగ:
    AstCoverpoint(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు_సమాసం* exprp,
                  Ast_కణుపు_సమాసం* iffp = nullptr, Ast_కణుపు* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        కూడు_Binsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};

// === AstNodeGen ===
class Ast_Gen_ఖండం final : బహిరంగ Ast_కణుపు_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    స్థిరరాశి bool m_unnamed;  // Originally unnamed (name change does not affect this)
    స్థిరరాశి bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

బహిరంగ:
    Ast_Gen_ఖండం(File_పంక్తి* fl, స్థిరరాశి string& name, Ast_కణుపు* itemsp, bool implied)
        : ASTGEN_SUPER_Gen_ఖండం(fl)
        , m_name{name}
        , m_unnamed{name.ఖాళీలు()}
        , m_implied{implied} {
        this->కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_ఖండం;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    std::string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
    శూన్య name(స్థిరరాశి std::string& name) override { m_name = name; }
    bool unnamed() స్థిరరాశి { తిరిగి m_unnamed; }
    bool implied() స్థిరరాశి { తిరిగి m_implied; }
    AstDefaultDisable* defaultDisablep() స్థిరరాశి { తిరిగి m_defaultDisablep; }
    శూన్య defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(స్థిరరాశి string& outer) స్థిరరాశి;
};
class AstGenCase final : బహిరంగ Ast_కణుపు_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
బహిరంగ:
    AstGenCase(File_పంక్తి* fl, Ast_కణుపు_సమాసం* exprp, Ast_Gen_Case_అంశం* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : బహిరంగ Ast_కణుపు_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
బహిరంగ:
    AstGenFor(File_పంక్తి* fl, Ast_కణుపు* initsp, Ast_కణుపు_సమాసం* condp, Ast_కణుపు* incsp, Ast_కణుపు* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->కూడు_Initsp(initsp);
        this->condp(condp);
        this->కూడు_Incsp(incsp);
        this->కూడు_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : బహిరంగ Ast_కణుపు_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
బహిరంగ:
    AstGenIf(File_పంక్తి* fl, Ast_కణుపు_సమాసం* condp, Ast_కణుపు* thensp, Ast_కణుపు* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->కూడు_Thensp(thensp);
        this->కూడు_Elsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : బహిరంగ Ast_కణుపు_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtype_Name_నిండిన;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_అంకె;  // Declaration token number
    VBaseOverride m_పునాది_Override;  // BaseOverride (inital/final/extends)
    bool m_hasRandVarsUpdate = false;  // Has updateRandVars method,
                                       // which updates pointers to rand variables in clone()
    bool m_covergroup = false;  // Is covergroup (TODO perhaps make a new Ast node type for CG?)
    bool m_extended = false;  // Is extension or extended by other classes
    bool m_interfaceClass = false;  // Interface class
    bool m_needRNG = false;  // Need RNG, uses srandom/randomize
    bool m_use_Virtual_ప్రజా = false;  // Subclasses need virtual public as uses interface class
    bool m_virtual = false;  // Virtual class
    bool m_printedFrom = false;  // This class is printed from i.e. is used as format arg.
    // Mutable, as only a recursion guard of the const dtypeName(), so also not dumped
    mutable bool m_dtypeNameBusy = false;  // In dtypeName(), which a parameter may lead back to
    // Covergroup options (when m_covergroup is true)
    పూర్ణాంకం m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool నిండిన_2, స్థిరరాశి string& scope) స్థిరరాశి;
    string dtypeNameScope() స్థిరరాశి;  // Prefix of dtypeName(true) for the scope declaring the class

బహిరంగ:
    AstClass(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_అంకె{fl->token_అంకె()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() స్థిరరాశి override { తిరిగి isCovergroup() ? "covergroup" : "class"; }
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి true; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    bool timescaleMatters() స్థిరరాశి override { తిరిగి false; }
    AstClassPackage* classOrPackagep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_classOrPackagep; }
    శూన్య classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() స్థిరరాశి VL_MT_STABLE {
        తిరిగి m_covergroupEnclosingClassp;
    }
    శూన్య covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_కణుపు* membersp() స్థిరరాశి VL_MT_STABLE { తిరిగి stmtsp(); }
    శూన్య కూడు_Membersp(Ast_కణుపు* nodep) { కూడు_Stmtsp(nodep); }
    bool hasRandVarsUpdate() స్థిరరాశి { తిరిగి m_hasRandVarsUpdate; }
    శూన్య hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() స్థిరరాశి { తిరిగి m_covergroup; }
    శూన్య isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() స్థిరరాశి { తిరిగి m_extended; }
    శూన్య isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() స్థిరరాశి { తిరిగి m_interfaceClass; }
    శూన్య isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() స్థిరరాశి { తిరిగి m_virtual; }
    శూన్య isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() స్థిరరాశి { తిరిగి m_needRNG; }
    శూన్య needRNG(bool flag) { m_needRNG = flag; }
    bool use_Virtual_ప్రజా() స్థిరరాశి { తిరిగి m_use_Virtual_ప్రజా; }
    శూన్య use_Virtual_ప్రజా(bool flag) { m_use_Virtual_ప్రజా = flag; }
    శూన్య markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() స్థిరరాశి { తిరిగి m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool నిండిన_2) స్థిరరాశి;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtype_Name_లోపల(స్థిరరాశి string& scope) స్థిరరాశి;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    శూన్య dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() స్థిరరాశి { తిరిగి !m_dtype_Name_నిండిన.ఖాళీలు(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_సందేశం() స్థిరరాశి override { తిరిగి hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    స్థిర string embeddedCovergroupTypeName(స్థిరరాశి string& name) { తిరిగి "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() స్థిరరాశి;
    // Covergroup options accessors
    పూర్ణాంకం cgAutoBinMax() స్థిరరాశి { తిరిగి m_cgAutoBinMax; }
    శూన్య cgAutoBinMax(పూర్ణాంకం విలువ) { m_cgAutoBinMax = విలువ; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    స్థిర bool isClassExtendedFrom(స్థిరరాశి AstClass* refClassp, స్థిరరాశి AstClass* పునాది_Classp);
    uint32_t decl_Token_అంకె() స్థిరరాశి override { తిరిగి m_decl_Token_అంకె; }
    శూన్య decl_Token_అంకె_సమితి_Min(uint32_t token_అంకె) override {
        m_decl_Token_అంకె = std::min(m_decl_Token_అంకె, token_అంకె);
    }
    శూన్య పునాది_Override(స్థిరరాశి VBaseOverride& flag) { m_పునాది_Override = flag; }
    VBaseOverride పునాది_Override() స్థిరరాశి { తిరిగి m_పునాది_Override; }
    // Return the lowest class extended from, or this class
    AstClass* పునాది_Most_Classp();
    స్థిర bool is_Cacheable_బిడ్డ(స్థిరరాశి Ast_కణుపు* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    మూస <typename T_Callable>
    శూన్య foreachMember(స్థిరరాశి T_Callable& f) {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_కణుపు*>::విలువ
                && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        ఒకవేళ (స్థిరరాశి AstClassExtends* స్థిరరాశి cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        కొరకు (Ast_కణుపు* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (Ast_కణుపు::వ్యక్తిగత_Type_Test<T_కణుపు>(stmtp)) f(this, static_cast<T_కణుపు*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    మూస <typename T_Callable>
    bool existsMember(స్థిరరాశి T_Callable& p) స్థిరరాశి {
        using T_కణుపు = typename Function_Arg_No_సూచిక_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, స్థిరరాశి AstClass*, స్థిరరాశి T_కణుపు*>::విలువ
                && std::is_పునాది_యొక్క<Ast_కణుపు, T_కణుపు>::విలువ,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        ఒకవేళ (స్థిరరాశి AstClassExtends* స్థిరరాశి cextendsp = this->extendsp()) {
            ఒకవేళ (cextendsp->classp()->existsMember(p)) తిరిగి true;
        }
        కొరకు (Ast_కణుపు* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (Ast_కణుపు::వ్యక్తిగత_Type_Test<T_కణుపు>(stmtp)) {
                ఒకవేళ (p(this, static_cast<T_కణుపు*>(stmtp))) తిరిగి true;
            }
        }
        తిరిగి false;
    }
};
class AstClassPackage final : బహిరంగ Ast_కణుపు_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
బహిరంగ:
    AstClassPackage(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() స్థిరరాశి override { తిరిగి "classpackage"; }
    bool timescaleMatters() స్థిరరాశి override { తిరిగి false; }
    AstClass* classp() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_classp; }
    శూన్య classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : బహిరంగ Ast_కణుపు_Module {
    // An interface declaration
    string m_dtype_Name_నిండిన;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
బహిరంగ:
    AstIface(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    string verilogKwd() స్థిరరాశి override { తిరిగి "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() స్థిరరాశి override { తిరిగి false; }
    bool hasVirtualRef() స్థిరరాశి { తిరిగి m_hasVirtualRef; }
    శూన్య సమితి_Has_Virtual_Ref() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool నిండిన_2) స్థిరరాశి;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    శూన్య dtypeNameFreeze();
};
class AstModule final : బహిరంగ Ast_కణుపు_Module {
    // A module declaration
    స్థిరరాశి bool m_isChecker;  // Module represents a checker
    స్థిరరాశి bool m_is_కార్యక్రమం;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

బహిరంగ:
    class Checker {};  // for constructor type-overload selection
    class కార్యక్రమం {};  // for constructor type-overload selection
    AstModule(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_కార్యక్రమం{false} {}
    AstModule(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_is_కార్యక్రమం{false} {}
    AstModule(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname, కార్యక్రమం)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_కార్యక్రమం{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() స్థిరరాశి override {
        తిరిగి m_isChecker ? "checker" : m_is_కార్యక్రమం ? "program" : "module";
    }
    bool timescaleMatters() స్థిరరాశి override { తిరిగి true; }
    bool isChecker() స్థిరరాశి { తిరిగి m_isChecker; }
    bool is_కార్యక్రమం() స్థిరరాశి { తిరిగి m_is_కార్యక్రమం; }
    bool hasGenericIface() స్థిరరాశి { తిరిగి m_hasGenericIface; }
    శూన్య hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
};
class Ast_Not_దొరికింది_Module final : బహిరంగ Ast_కణుపు_Module {
    // A missing module declaration
బహిరంగ:
    Ast_Not_దొరికింది_Module(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : ASTGEN_SUPER_Not_దొరికింది_Module(fl, name, libname) {}
    ASTGEN_MEMBERS_Ast_Not_దొరికింది_Module;
    string verilogKwd() స్థిరరాశి override { తిరిగి "/*not-found-*/ module"; }
    bool timescaleMatters() స్థిరరాశి override { తిరిగి false; }
};
class AstPackage final : బహిరంగ Ast_కణుపు_Module {
    // A package declaration
బహిరంగ:
    AstPackage(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() స్థిరరాశి override { తిరిగి "package"; }
    bool timescaleMatters() స్థిరరాశి override { తిరిగి !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : బహిరంగ Ast_కణుపు_Module {
    // A primitive declaration
బహిరంగ:
    AstPrimitive(File_పంక్తి* fl, స్థిరరాశి string& name, స్థిరరాశి string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() స్థిరరాశి override { తిరిగి "primitive"; }
    bool timescaleMatters() స్థిరరాశి override { తిరిగి false; }
};

// === AstNodeProcedure ===
class AstAlways final : బహిరంగ Ast_కణుపు_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    స్థిరరాశి VAlwaysKwd m_keyword;

బహిరంగ:
    AstAlways(File_పంక్తి* fl, VAlwaysKwd keyword, Ast_Sen_వృక్షం* sentreep, Ast_కణుపు* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit అంతఃవరుస AstAlways(Ast_కేటాయించు_W* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    VAlwaysKwd keyword() స్థిరరాశి { తిరిగి m_keyword; }
};
class AstAlwaysObserved final : బహిరంగ Ast_కణుపు_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

బహిరంగ:
    AstAlwaysObserved(File_పంక్తి* fl, Ast_Sen_వృక్షం* sentreep, Ast_కణుపు* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : బహిరంగ Ast_కణుపు_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

బహిరంగ:
    explicit AstAlwaysPost(File_పంక్తి* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : బహిరంగ Ast_కణుపు_Procedure {
    // Like always but Postponed scheduling region

బహిరంగ:
    AstAlwaysPostponed(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : బహిరంగ Ast_కణుపు_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

బహిరంగ:
    explicit AstAlwaysPre(File_పంక్తి* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : బహిరంగ Ast_కణుపు_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

బహిరంగ:
    AstAlwaysReactive(File_పంక్తి* fl, Ast_Sen_వృక్షం* sentreep, Ast_కణుపు* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : బహిరంగ Ast_కణుపు_Procedure {
బహిరంగ:
    AstFinal(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : బహిరంగ Ast_కణుపు_Procedure {
బహిరంగ:
    AstInitial(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : బహిరంగ Ast_కణుపు_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
బహిరంగ:
    AstInitialAutomatic(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : బహిరంగ Ast_కణుపు_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
బహిరంగ:
    AstInitialStatic(File_పంక్తి* fl, Ast_కణుపు* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : బహిరంగ Ast_కణుపు_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
బహిరంగ:
    AstBracketRange(File_పంక్తి* fl, Ast_కణుపు* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    మాయా string emitC() { V3ERROR_NA_RETURN(""); }
    మాయా string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_గమ్యం() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి false; }
};
class AstRange final : బహిరంగ Ast_కణుపు_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    స్థిరరాశి bool m_fromBracket = false;  // From C-style '[x]' declaration
బహిరంగ:
    AstRange(File_పంక్తి* fl, Ast_కణుపు_సమాసం* leftp, Ast_కణుపు_సమాసం* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    అంతఃవరుస AstRange(File_పంక్తి* fl, పూర్ణాంకం ఎడమ_2, పూర్ణాంకం కుడి_2);
    అంతఃవరుస AstRange(File_పంక్తి* fl, స్థిరరాశి VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    అంతఃవరుస పూర్ణాంకం ఎడమ_Const() స్థిరరాశి VL_MT_STABLE;
    అంతఃవరుస పూర్ణాంకం కుడి_Const() స్థిరరాశి VL_MT_STABLE;
    పూర్ణాంకం hiConst() స్థిరరాశి VL_MT_STABLE {
        స్థిరరాశి పూర్ణాంకం l = ఎడమ_Const();
        స్థిరరాశి పూర్ణాంకం r = కుడి_Const();
        తిరిగి l > r ? l : r;
    }
    పూర్ణాంకం loConst() స్థిరరాశి VL_MT_STABLE {
        స్థిరరాశి పూర్ణాంకం l = ఎడమ_Const();
        స్థిరరాశి పూర్ణాంకం r = కుడి_Const();
        తిరిగి l > r ? r : l;
    }
    పూర్ణాంకం elementsConst() స్థిరరాశి VL_MT_STABLE { తిరిగి hiConst() - loConst() + 1; }
    bool ascending() స్థిరరాశి { తిరిగి ఎడమ_Const() < కుడి_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_ఎడమ_గమ్యం_కుడి() స్థిరరాశి {
        తిరిగి VNumRange{ఎడమ_Const(), కుడి_Const()}.seq_ఎడమ_గమ్యం_కుడి();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_కుడి_గమ్యం_ఎడమ() స్థిరరాశి {
        తిరిగి VNumRange{ఎడమ_Const(), కుడి_Const()}.seq_కుడి_గమ్యం_ఎడమ();
    }
    శూన్య dump(std::ostream& str) స్థిరరాశి override;
    శూన్య dumpJson(std::ostream& str) స్థిరరాశి override;
    మాయా string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* samep) స్థిరరాశి override {
        స్థిరరాశి AstRange* స్థిరరాశి asamep = VN_DBG_AS(samep, Range);
        తిరిగి fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() స్థిరరాశి { తిరిగి m_fromBracket; }
};
class AstUnsizedRange final : బహిరంగ Ast_కణుపు_Range {
    // Unsized range specification, for open arrays
బహిరంగ:
    explicit AstUnsizedRange(File_పంక్తి* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    మాయా string emitC() { V3ERROR_NA_RETURN(""); }
    మాయా string emitVerilog() { తిరిగి "[]"; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};
class AstWildcardRange final : బహిరంగ Ast_కణుపు_Range {
    // Wildcard range specification, for wildcard index type associative arrays
బహిరంగ:
    explicit AstWildcardRange(File_పంక్తి* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    మాయా string emitC() { V3ERROR_NA_RETURN(""); }
    మాయా string emitVerilog() { తిరిగి "[*]"; }
    bool same_కణుపు(స్థిరరాశి Ast_కణుపు* /*samep*/) స్థిరరాశి override { తిరిగి true; }
};

#అయితే_ముగింపు  // Guard
