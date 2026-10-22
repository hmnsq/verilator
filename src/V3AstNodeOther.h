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

#که_نه_تعریف_شوی VERILATOR_V3ASTNODES_H_
#تعریف_کړه VERILATOR_V3ASTNODES_H_

#که_نه_تعریف_شوی VERILATOR_V3AST_H_
#تېروتنه "Use V3Ast.h as the include"
#ضمن "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#تعریف_کړه VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#د_که_پای

#ضمن <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_غوټه_Cons_توکی VL_NOT_FINAL : عام Ast_غوټه {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    ملګری class Ast_غوټه_بیان;  // For m_knownConstantTree
    mutable bool m_معلوم_Constant_ونه = false;  // Known to be a tree of constants

محمي:
    Ast_غوټه_Cons_توکی(VNType t, File_کرښه* fl, Ast_غوټه_بیان* valuep)
        : Ast_غوټه{t, fl} {
        this->valuep(valuep);
    }

عام:
    ASTGEN_MEMBERS_Ast_غوټه_Cons_توکی;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class Ast_غوټه_Cover_Decl VL_NOT_FINAL : عام Ast_غوټه {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_پاڼه;  // Coverage point's page tag
    string m_متن;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    صحيح m_bin_عدد = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    صحيح m_local_Bin_عدد = 0;  // Per-module coverage bin offset
    bool m_فی_Instance = false;  // Don't clone during inlining, already per instance
عام:
    Ast_غوټه_Cover_Decl(VNType t, File_کرښه* fl, ثابت string& پاڼه, ثابت string& comment)
        : Ast_غوټه(t, fl)
        , m_پاڼه{پاڼه}
        , m_متن{comment} {}
    ASTGEN_MEMBERS_Ast_غوټه_Cover_Decl;
    ثابت حرف* broken() ثابت override {
        إذا (m_معلومات_Declp
            && (m_معلومات_Declp == this || m_معلومات_Declp->m_معلومات_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvt_منزل_Hex(this));
        }
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح instr_شمېر() ثابت override { أرجع 1 + 2 * INSTR_شمېر_LD; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    صحيح bin_عدد() ثابت { أرجع m_bin_عدد; }
    فراغ bin_عدد(صحيح flag) { m_bin_عدد = flag; }
    صحيح local_Bin_عدد() ثابت { أرجع m_local_Bin_عدد; }
    فراغ local_Bin_عدد(صحيح flag) { m_local_Bin_عدد = flag; }
    bool فی_Instance() ثابت { أرجع m_فی_Instance; }
    فراغ فی_Instance(bool flag) { m_فی_Instance = flag; }
    مجازي صحيح اندازه() ثابت = 0;
    ثابت string& comment() ثابت { أرجع m_متن; }  // text to insert in code
    ثابت string& پاڼه() ثابت { أرجع m_پاڼه; }
    ثابت string& hier() ثابت { أرجع m_hier; }
    فراغ hier(ثابت string& flag) { m_hier = flag; }
    فراغ comment(ثابت string& flag) { m_متن = flag; }
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        ثابت Ast_غوټه_Cover_Decl* ثابت asamep = VN_DBG_AS(samep, غوټه_Cover_Decl);
        أرجع (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && پاڼه() == asamep->پاڼه()
                && فی_Instance() == asamep->فی_Instance());
    }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    فراغ معلومات_Declp(Ast_غوټه_Cover_Decl* nodep) { m_معلومات_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_غوټه_Cover_Decl* معلومات_Decl_Nullp() ثابت { أرجع m_معلومات_Declp; }
    Ast_غوټه_Cover_Decl* معلومات_Decl_Thisp() { أرجع معلومات_Decl_Nullp() ? معلومات_Decl_Nullp() : this; }
};
class Ast_غوټه_FTask VL_NOT_FINAL : عام Ast_غوټه {
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
    string m_iface_پورټ_Name;  // Interface port name for out-of-block definition (IEEE 25.8)
    uint64_t m_dpi_Open_پلار = 0;  // DPI import open array, if !=0, how many callees
    bool m_task_عامه : 1;  // Public task
    bool m_class_لاره : 1;  // Class method
    bool m_didProto : 1;  // Did prototype processing
    bool m_prototype : 1;  // Just a prototype
    bool m_dpiExport : 1;  // DPI exported
    bool m_dpiImport : 1;  // DPI imported
    bool m_dpiContext : 1;  // DPI import context
    bool m_dpi_Open_ماشوم : 1;  // DPI import open array child wrapper
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
    VBaseOverride m_بنسټ_Override;  // BaseOverride (inital/final/extends)
    VLifetime m_lifetime;  // Default lifetime of local vars
    VIsCached m_purity;  // Pure state

محمي:
    Ast_غوټه_FTask(VNType t, File_کرښه* fl, ثابت string& name, Ast_غوټه* stmtsp)
        : Ast_غوټه{t, fl}
        , m_name{name}
        , m_task_عامه{false}
        , m_class_لاره{false}
        , m_didProto{false}
        , m_prototype{false}
        , m_dpiExport{false}
        , m_dpiImport{false}
        , m_dpiContext{false}
        , m_dpi_Open_ماشوم{false}
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
        جمع_کړه_Stmtsp(stmtsp);
        cname(name);  // Might be overridden by dpi import/export
    }

عام:
    ASTGEN_MEMBERS_Ast_غوټه_FTask;
    مجازي Ast_غوټه_FTask* cloneType(ثابت string& name) = 0;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    bool isGateOptimizable() ثابت override {
        أرجع !((m_dpiExport || m_dpiImport) && !m_dpi_خالص);
    }
    // {AstFunc only} op1 = Range output variable
    فراغ name(ثابت string& name) override { m_name = name; }
    string cname() ثابت { أرجع m_cname; }
    فراغ cname(ثابت string& cname) { m_cname = cname; }
    bool isFunction() ثابت { أرجع fvarp() != nullptr; }
    // MORE ACCESSORS
    فراغ dpi_Open_پلار_Inc() { ++m_dpi_Open_پلار; }
    فراغ dpi_Open_پلار_Clear() { m_dpi_Open_پلار = 0; }
    uint64_t dpi_Open_پلار() ثابت { أرجع m_dpi_Open_پلار; }
    bool task_عامه() ثابت { أرجع m_task_عامه; }
    فراغ task_عامه(bool flag) { m_task_عامه = flag; }
    bool class_لاره() ثابت { أرجع m_class_لاره; }
    فراغ class_لاره(bool flag) { m_class_لاره = flag; }
    bool didProto() ثابت { أرجع m_didProto; }
    فراغ didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ثابت { أرجع m_isExternProto; }
    فراغ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ثابت { أرجع m_isExternDef; }
    فراغ isExternDef(bool flag) { m_isExternDef = flag; }
    ثابت string& iface_پورټ_Name() ثابت { أرجع m_iface_پورټ_Name; }
    فراغ iface_پورټ_Name(ثابت string& name) { m_iface_پورټ_Name = name; }
    bool prototype() ثابت { أرجع m_prototype; }
    فراغ prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ثابت { أرجع m_dpiExport; }
    فراغ dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ثابت { أرجع m_dpiImport; }
    فراغ dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ثابت { أرجع m_dpiContext; }
    فراغ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpi_Open_ماشوم() ثابت { أرجع m_dpi_Open_ماشوم; }
    فراغ dpi_Open_ماشوم(bool flag) { m_dpi_Open_ماشوم = flag; }
    bool dpiTask() ثابت { أرجع m_dpiTask; }
    فراغ dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ثابت { أرجع !m_dpiCDecl.تشه(); }
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
    فراغ سېټ_Need_Process() { m_needProcess = true; }
    bool isCovergroupSample() ثابت { أرجع m_isCovergroupSample; }
    فراغ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    فراغ بنسټ_Override(ثابت VBaseOverride& flag) { m_بنسټ_Override = flag; }
    VBaseOverride بنسټ_Override() ثابت { أرجع m_بنسټ_Override; }
    فراغ lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ثابت { أرجع m_lifetime; }
    bool is_خالص() override;
    ثابت حرف* broken() ثابت override;
    فراغ propagateAttrFrom(ثابت Ast_غوټه_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        class_لاره(fromp->class_لاره());
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
class Ast_غوټه_File VL_NOT_FINAL : عام Ast_غوټه {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
عام:
    Ast_غوټه_File(VNType t, File_کرښه* fl, ثابت string& name)
        : Ast_غوټه{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_غوټه_File;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool same_غوټه(ثابت Ast_غوټه* /*samep*/) ثابت override { أرجع true; }
};
class Ast_غوټه_Func_Cov_توکی VL_NOT_FINAL : عام Ast_غوټه {
    // Base class for functional coverage items (coverpoints, crosses)
محمي:
    string m_name;  // Item name

عام:
    Ast_غوټه_Func_Cov_توکی(VNType t, File_کرښه* fl, ثابت string& name)
        : Ast_غوټه{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_غوټه_Func_Cov_توکی;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    bool maybe_Pointed_منزل() ثابت override { أرجع true; }
};
class Ast_غوټه_Gen VL_NOT_FINAL : عام Ast_غوټه {
    // Generate construct
عام:
    Ast_غوټه_Gen(VNType t, File_کرښه* fl)
        : Ast_غوټه{t, fl} {}
    ASTGEN_MEMBERS_Ast_غوټه_Gen;
};
class Ast_غوټه_Module VL_NOT_FINAL : عام Ast_غوټه {
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
    صحيح m_ژوروالی = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    صحيح m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
    VLifetime m_lifetime;  // Lifetime
    VTimescale m_timeunit;  // Global time unit
    VOptionBool m_unconnectedDrive;  // State of `unconnected_drive
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

    bool m_mod_عامه : 1;  // Module has public references
    bool m_modTrace : 1;  // Tracing this module
    bool m_دننه_Library : 1;  // From a library, no error if not used, never top level
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
محمي:
    Ast_غوټه_Module(VNType t, File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : Ast_غوټه{t, fl}
        , m_name{name}
        , m_origName{name}
        , m_libname{libname}
        , m_mod_عامه{false}
        , m_modTrace{false}
        , m_دننه_Library{false}
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

عام:
    ASTGEN_MEMBERS_Ast_غوټه_Module;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
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
    string someInstanceName() ثابت VL_MT_خوندي { أرجع m_someInstanceName; }
    فراغ someInstanceName(ثابت string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ثابت { أرجع m_defaultDisablep; }
    فراغ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool دننه_Library() ثابت { أرجع m_دننه_Library; }
    فراغ دننه_Library(bool flag) { m_دننه_Library = flag; }
    فراغ ژوروالی(صحيح ارزښت) { m_ژوروالی = ارزښت; }
    صحيح ژوروالی() ثابت VL_MT_خوندي { أرجع m_ژوروالی; }
    فراغ level(صحيح ارزښت) { m_level = ارزښت; }
    صحيح level() ثابت VL_MT_خوندي { أرجع m_level; }
    string libname() ثابت { أرجع m_libname; }
    bool is_سر() ثابت VL_MT_خوندي { أرجع level() == 1; }
    bool mod_عامه() ثابت { أرجع m_mod_عامه; }
    فراغ mod_عامه(bool flag) { m_mod_عامه = flag; }
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
    bool hier_بلاک() ثابت { أرجع m_hier_بلاک; }
    فراغ hier_بلاک(bool flag) { m_hier_بلاک = flag; }
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
class Ast_غوټه_Procedure VL_NOT_FINAL : عام Ast_غوټه {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
محمي:
    Ast_غوټه_Procedure(VNType t, File_کرښه* fl, Ast_غوټه* stmtsp)
        : Ast_غوټه{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        جمع_کړه_Stmtsp(stmtsp);
    }

عام:
    ASTGEN_MEMBERS_Ast_غوټه_Procedure;
    // METHODS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool isJustOneBodyStmt() ثابت { أرجع stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ثابت { أرجع m_suspendable; }
    فراغ سېټ_Suspendable() { m_suspendable = true; }
    bool needProcess() ثابت { أرجع m_needProcess; }
    فراغ سېټ_Need_Process() { m_needProcess = true; }
};
class Ast_غوټه_Range VL_NOT_FINAL : عام Ast_غوټه {
    // A range, sized or unsized
محمي:
    Ast_غوټه_Range(VNType t, File_کرښه* fl)
        : Ast_غوټه{t, fl} {}

عام:
    ASTGEN_MEMBERS_Ast_غوټه_Range;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_فعال final : عام Ast_غوټه {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

عام:
    Ast_فعال(File_کرښه* fl, ثابت string& name, Ast_Sen_ونه* sentreep)
        : ASTGEN_SUPER_فعال(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_Ast_فعال;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    // Statements are broken into pieces, as some must come before others.
    فراغ sentreep(Ast_Sen_ونه* nodep) { m_sentreep = nodep; }
    Ast_Sen_ونه* sentreep() ثابت { أرجع m_sentreep; }
    // METHODS
    مضمن bool hasClocked() ثابت;
    مضمن bool hasCombo() ثابت;
    مضمن bool hasInitial() ثابت;
    مضمن bool hasStatic() ثابت;
};
class AstAlias final : عام Ast_غوټه {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
عام:
    AstAlias(File_کرښه* fl, Ast_غوټه_بیان* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : عام Ast_غوټه {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
عام:
    AstAliasScope(File_کرښه* fl, Ast_غوټه_بیان* lhsp, Ast_غوټه_بیان* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : عام Ast_غوټه {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
عام:
    AstBind(File_کرښه* fl, ثابت string& name, Ast_غوټه* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        جمع_کړه_Cellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Bind Target name
    فراغ name(ثابت string& name) override { m_name = name; }
};
class AstCFunc final : عام Ast_غوټه {
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
    bool m_decl_شخصي : 1;  // Declare it private
    bool m_keep_If_تشه : 1;  // Keep declaration and definition separate, even if empty
    bool m_slow : 1;  // Slow routine, called once or just at init time
    bool m_func_عامه : 1;  // From user public task/function
    bool m_isConstructor : 1;  // Is C class constructor
    bool m_isDestructor : 1;  // Is C class destructor
    bool m_is_لاره : 1;  // Is inside a class definition
    bool m_isLoose : 1;  // Semantically this is a method, but is implemented as a function with
                         // an explicitly passed 'self' pointer as the first argument.  This can
                         // be slightly faster due to __restrict, and we do not declare in header
                         // so adding/removing loose functions doesn't recompile everything.
    bool m_isVirtual : 1;  // Virtual function
    bool m_ثبت_ټکی : 1;  // User may call into this top level function
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
    صحيح m_لګښت;  // Function call cost
عام:
    AstCFunc(File_کرښه* fl, ثابت string& name, AstScope* scopep, ثابت string& rtnType = "")
        : ASTGEN_SUPER_CFunc(fl) {
        m_isConst = VBoolOrUnknown::BU_UNKNOWN;  // Unknown until analyzed
        m_scopep = scopep;
        m_name = name;
        m_rtnType = rtnType;
        m_isStatic = false;
        m_isTrace = false;
        m_dontCombine = false;
        m_decl_شخصي = false;
        m_keep_If_تشه = false;
        m_slow = false;
        m_func_عامه = false;
        m_isConstructor = false;
        m_isDestructor = false;
        m_is_لاره = true;
        m_isLoose = false;
        m_isVirtual = false;
        m_needProcess = false;
        m_ثبت_ټکی = false;
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
        m_لګښت = v3Global.opt.instr_شمېر_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        ثابت AstCFunc* ثابت asamep = VN_DBG_AS(samep, CFunc);
        أرجع ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    فراغ name(ثابت string& name) override { m_name = name; }
    صحيح instr_شمېر() ثابت override { أرجع m_لګښت; }
    VBoolOrUnknown isConst() ثابت { أرجع m_isConst; }
    فراغ isConst(bool flag) { m_isConst.سېټ_True_Or_False(flag); }
    فراغ isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ثابت { أرجع m_isStatic; }
    فراغ isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ثابت VL_MT_خوندي { أرجع m_isTrace; }
    فراغ isTrace(bool flag) { m_isTrace = flag; }
    فراغ cname(ثابت string& name) { m_cname = name; }
    string cname() ثابت { أرجع m_cname; }
    AstScope* scopep() ثابت { أرجع m_scopep; }
    فراغ scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ثابت { أرجع ((m_rtnType == "") ? "void" : m_rtnType); }
    فراغ rtnType(ثابت string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ثابت { أرجع m_dontCombine || isTrace() || ثبت_ټکی(); }
    فراغ dontCombine(bool flag) { m_dontCombine = flag; }
    bool decl_شخصي() ثابت { أرجع m_decl_شخصي; }
    فراغ decl_شخصي(bool flag) { m_decl_شخصي = flag; }
    bool keep_If_تشه() ثابت VL_MT_خوندي { أرجع m_keep_If_تشه; }
    فراغ keep_If_تشه(bool flag) { m_keep_If_تشه = flag; }
    bool slow() ثابت VL_MT_خوندي { أرجع m_slow; }
    فراغ slow(bool flag) { m_slow = flag; }
    bool func_عامه() ثابت { أرجع m_func_عامه; }
    فراغ func_عامه(bool flag) { m_func_عامه = flag; }
    فراغ argTypes(ثابت string& str) { m_argTypes = str; }
    string argTypes() ثابت { أرجع m_argTypes; }
    فراغ ifdef(ثابت string& str) { m_ifdef = str; }
    string ifdef() ثابت { أرجع m_ifdef; }
    bool isConstructor() ثابت { أرجع m_isConstructor; }
    فراغ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ثابت { أرجع m_isDestructor; }
    فراغ isDestructor(bool flag) { m_isDestructor = flag; }
    bool is_لاره() ثابت { أرجع m_is_لاره; }
    فراغ is_لاره(bool flag) { m_is_لاره = flag; }
    bool isLoose() ثابت { أرجع m_isLoose; }
    فراغ isLoose(bool flag) { m_isLoose = flag; }
    bool is_Proper_لاره() ثابت { أرجع is_لاره() && !isLoose(); }
    bool isVirtual() ثابت { أرجع m_isVirtual; }
    فراغ isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ثابت { أرجع m_needProcess; }
    فراغ سېټ_Need_Process() { m_needProcess = true; }
    bool ثبت_ټکی() ثابت { أرجع m_ثبت_ټکی; }
    فراغ ثبت_ټکی(bool flag) { m_ثبت_ټکی = flag; }
    bool dpi_خالص() ثابت { أرجع m_dpi_خالص; }
    فراغ dpi_خالص(bool flag) { m_dpi_خالص = flag; }
    bool dpiContext() ثابت { أرجع m_dpiContext; }
    فراغ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ثابت VL_MT_خوندي { أرجع m_dpiExportDispatcher; }
    فراغ dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ثابت { أرجع m_dpiExportImpl; }
    فراغ dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ثابت VL_MT_خوندي { أرجع m_dpiImportPrototype; }
    فراغ dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ثابت { أرجع m_dpiImportWrapper; }
    فراغ dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ثابت { أرجع !m_dpiCDecl.تشه(); }
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
    فراغ لګښت(صحيح لګښت) { m_لګښت = لګښت; }
    // Special methods
    bool تشه_Body() ثابت {
        أرجع !keep_If_تشه() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : عام Ast_غوټه {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ثابت string m_name;
    ثابت VUseType m_useType;  // What sort of use this is

عام:
    AstCUse(File_کرښه* fl, VUseType useType, ثابت string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    VUseType useType() ثابت { أرجع m_useType; }
};
class AstCell final : عام Ast_غوټه {
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
    File_کرښه* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    صحيح m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
عام:
    AstCell(File_کرښه* fl, File_کرښه* mfl, ثابت string& instName, ثابت string& modName,
            AstPin* pinsp, AstPin* paramsp, AstRange* rangep)
        : ASTGEN_SUPER_Cell(fl)
        , m_modNameFileline{mfl}
        , m_name{instName}
        , m_origName{instName}
        , m_modName{modName}
        , m_hasIfaceVar{false}
        , m_recursive{false}
        , m_trace{true} {
        جمع_کړه_Pinsp(pinsp);
        جمع_کړه_Paramsp(paramsp);
        جمع_کړه_Rangep(rangep);
    }
    ASTGEN_MEMBERS_AstCell;
    // No cloneRelink, we presume cloneee's want the same module linkages
    فراغ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Cell name
    فراغ name(ثابت string& name) override { m_name = name; }
    string origName() ثابت override { أرجع m_origName; }  // * = Original name
    فراغ origName(ثابت string& name) { m_origName = name; }
    string modName() ثابت { أرجع m_modName; }  // * = Instance name
    فراغ modName(ثابت string& name) { m_modName = name; }
    File_کرښه* modNameFileline() ثابت { أرجع m_modNameFileline; }
    Ast_غوټه_Module* modp() ثابت { أرجع m_modp; }  // [AfterLink] = Pointer to module instantiated
    فراغ modp(Ast_غوټه_Module* nodep) { m_modp = nodep; }
    صحيح arrayIdx() ثابت { أرجع m_arrayIdx; }
    فراغ arrayIdx(صحيح idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ثابت { أرجع m_hasIfaceVar; }
    فراغ hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    فراغ trace(bool flag) { m_trace = flag; }
    bool isTrace() ثابت { أرجع m_trace; }
    فراغ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ثابت { أرجع m_recursive; }
};
class AstCellInline final : عام Ast_غوټه {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ثابت std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ثابت std::string m_origModName;  // __BEGIN__ or __FORK__
عام:
    AstCellInline(File_کرښه* fl, ثابت std::string& name, ثابت std::string& origModName)
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
class AstCellInlineScope final : عام Ast_غوټه {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ثابت std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
عام:
    AstCellInlineScope(File_کرښه* fl, ثابت std::string& name, ثابت std::string& origModName)
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
class Ast_Cg_Option_ټاکل final : عام Ast_غوټه {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ثابت VCoverOptionType m_optType;  // Option type
    ثابت string m_name;  // Original option name (for diagnostics on unknown options)
    ثابت bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
عام:
    Ast_Cg_Option_ټاکل(File_کرښه* fl, bool typeOption, VCoverOptionType optType,
                      ثابت string& rawName, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cg_Option_ټاکل(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Cg_Option_ټاکل;
    // ACCESSORS
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    VCoverOptionType optType() ثابت { أرجع m_optType; }
    bool typeOption() ثابت { أرجع m_typeOption; }
};
class AstClassExtends final : عام Ast_غوټه {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ثابت bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

عام:
    AstClassExtends(File_کرښه* fl, Ast_غوټه* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    string verilogKwd() ثابت override { أرجع isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ثابت;
    AstClass* classp() ثابت;  // Like above, but throws error if nulll
    bool isImplements() ثابت { أرجع m_isImplements; }
    فراغ parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ثابت { أرجع m_parameterized; }
};
class AstClocking final : عام Ast_غوټه {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ثابت bool m_isGlobal;  // True if global clocking

عام:
    AstClocking(File_کرښه* fl, ثابت std::string& name, Ast_Sen_ونه* sentreep, Ast_غوټه* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool isDefault() ثابت { أرجع m_isDefault; }
    bool isGlobal() ثابت { أرجع m_isGlobal; }
    AstVar* ensureEventp(bool ماشوم_DType = false);
    فراغ makeDefault() { m_isDefault = true; }
};
class Ast_Clocking_توکی final : عام Ast_غوټه {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

عام:
    Ast_Clocking_توکی(File_کرښه* fl, VDirection direction, Ast_غوټه_بیان* skewp, Ast_غوټه* clockingDeclp)
        : ASTGEN_SUPER_Clocking_توکی(fl) {
        m_direction = direction;
        this->skewp(skewp);
        إذا (Ast_ټاکل* ثابت clkAssignp = VN_CAST(clockingDeclp, ټاکل)) {
            this->assignp(clkAssignp);
        } وإلا {
            exprp(VN_AS(clockingDeclp, غوټه_بیان));
        }
    }
    ASTGEN_MEMBERS_Ast_Clocking_توکی;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VDirection direction() ثابت { أرجع m_direction; }
    Ast_Clocking_توکی* outputp() ثابت { أرجع m_outputp; }
    فراغ outputp(Ast_Clocking_توکی* outputp) { m_outputp = outputp; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
};
class AstConfig final : عام Ast_غوټه {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

عام:
    AstConfig(File_کرښه* fl, ثابت std::string& libname, ثابت std::string& cellname)
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
class AstConfigCell final : عام Ast_غوټه {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

عام:
    AstConfigCell(File_کرښه* fl, ثابت std::string& libname, ثابت std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_libname + "." + m_cellname; }
    std::string libname() ثابت VL_MT_STABLE { أرجع m_libname; }
    std::string cellname() ثابت VL_MT_STABLE { أرجع m_cellname; }
};
class Ast_Config_قاعده final : عام Ast_غوټه {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ثابت bool m_isCell;  // Declared as "cell" versus "instance"

عام:
    Ast_Config_قاعده(File_کرښه* fl, Ast_غوټه* cellp, Ast_غوټه* usep, bool isCell)
        : ASTGEN_SUPER_Config_قاعده(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        جمع_کړه_Usep(usep);
    }
    ASTGEN_MEMBERS_Ast_Config_قاعده;
    bool isCell() ثابت VL_MT_STABLE { أرجع m_isCell; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstConfigUse final : عام Ast_غوټه {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ثابت bool m_isConfig;  // ":config"; Config, not module/primitive name

عام:
    AstConfigUse(File_کرښه* fl, ثابت std::string& libname, ثابت std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        جمع_کړه_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_libname + "." + m_cellname; }
    std::string libname() ثابت VL_MT_STABLE { أرجع m_libname; }
    std::string cellname() ثابت VL_MT_STABLE { أرجع m_cellname; }
    bool isConfig() ثابت VL_MT_STABLE { أرجع m_isConfig; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstConstraint final : عام Ast_غوټه {
    // Constraint
    // @astgen op1 := itemsp : List[AstNode]
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    string m_name;  // Name of constraint
    VBaseOverride m_بنسټ_Override;  // BaseOverride (inital/final/extends)
    bool m_isExternDef = false;  // Extern prototype definition
    bool m_isExternExplicit = false;  // Explicit prototype declaration (has extern)
    bool m_isExternProto = false;  // Prototype declaration (implicit or explicit)
    bool m_is_Kwd_خالص = false;  // Pure constraint
    bool m_isStatic = false;  // Static constraint
عام:
    AstConstraint(File_کرښه* fl, ثابت string& name, Ast_غوټه* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Scope name
    فراغ name(ثابت string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    bool same_غوټه(ثابت Ast_غوټه* /*samep*/) ثابت override { أرجع true; }
    فراغ بنسټ_Override(ثابت VBaseOverride& flag) { m_بنسټ_Override = flag; }
    VBaseOverride بنسټ_Override() ثابت { أرجع m_بنسټ_Override; }
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
class AstCoverBin final : عام Ast_غوټه {
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
    AstCoverBin(File_کرښه* fl, ثابت string& name, Ast_غوټه* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_غوټه_بیان* iffp = nullptr)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{isIllegal ? VCoverBinsType::BINS_ILLEGAL
                               : (isIgnore ? VCoverBinsType::BINS_IGNORE
                                           : (isWildcard ? VCoverBinsType::BINS_WILDCARD
                                                         : VCoverBinsType::BINS_USER))}
        , m_isWildcard{isWildcard} {
        جمع_کړه_Rangesp(rangesp);
        this->iffp(iffp);
    }
    // Constructor for automatic bins
    AstCoverBin(File_کرښه* fl, ثابت string& name, Ast_غوټه_بیان* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_کرښه* fl, ثابت string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_کرښه* fl, ثابت string& name, Ast_Cover_Trans_سېټ* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        جمع_کړه_Transp(transp);
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
class AstCoverBinsof final : عام Ast_غوټه {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ثابت bool m_isNegated;  // Complement the selection within the cross product

عام:
    AstCoverBinsof(File_کرښه* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_غوټه* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        جمع_کړه_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    bool isNegated() ثابت { أرجع m_isNegated; }
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverBinsof* ثابت asamep = VN_DBG_AS(samep, CoverBinsof);
        أرجع m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : عام Ast_غوټه {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ثابت string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ثابت VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

عام:
    AstCoverCrossBin(File_کرښه* fl, ثابت string& name, Ast_غوټه* selectp, Ast_غوټه_بیان* iffp,
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
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverCrossBin* ثابت asamep = VN_DBG_AS(samep, CoverCrossBin);
        أرجع m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : عام Ast_غوټه {
    // The complete bin-tuple space of the enclosing cross
    ثابت string m_name;  // Referenced cross name

عام:
    AstCoverCrossRef(File_کرښه* fl, ثابت string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {  // LCOV_EXCL_START
        أرجع m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : عام Ast_غوټه {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ثابت bool m_isOr;  // Union (||), rather than intersection (&&)

عام:
    AstCoverCrossSelect(File_کرښه* fl, Ast_غوټه* lhsp, Ast_غوټه* rhsp, bool isOr)
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
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {  // LCOV_EXCL_START
        أرجع m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : عام Ast_غوټه {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
عام:
    AstCoverCrossWith(File_کرښه* fl, Ast_غوټه* selectp, Ast_غوټه_بیان* filterp, Ast_غوټه_بیان* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : عام Ast_غوټه {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ثابت VCoverOptionType m_optType;  // Option being assigned
    ثابت bool m_typeOption;  // type_option (vs option)

عام:
    AstCoverOption(File_کرښه* fl, bool typeOption, VCoverOptionType optType, Ast_غوټه_بیان* valuep)
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
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {  // LCOV_EXCL_START
        ثابت AstCoverOption* ثابت asamep = VN_DBG_AS(samep, CoverOption);
        أرجع m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : عام Ast_غوټه {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
عام:
    AstCoverToggle(File_کرښه* fl, AstCoverInc* incp, Ast_غوټه_بیان* origp, Ast_غوټه_بیان* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    صحيح instr_شمېر() ثابت override { أرجع 3 + INSTR_شمېر_BRANCH + INSTR_شمېر_LD; }
    bool same_غوټه(ثابت Ast_غوټه* /*samep*/) ثابت override { أرجع true; }
    bool isGateOptimizable() ثابت override { أرجع false; }
    bool isPredictOptimizable() ثابت override { أرجع true; }
    bool isOutputter() override {
        أرجع false;  // Though the AstCoverInc under this is an outputter
    }
};
class Ast_Cover_Trans_توکی final : عام Ast_غوټه {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ثابت VTransRepType m_repType;

عام:
    Ast_Cover_Trans_توکی(File_کرښه* fl, Ast_غوټه* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_Cover_Trans_توکی(fl)
        , m_repType{repType} {
        جمع_کړه_Valuesp(valuesp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_توکی;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class Ast_Cover_Trans_سېټ final : عام Ast_غوټه {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
عام:
    Ast_Cover_Trans_سېټ(File_کرښه* fl, Ast_Cover_Trans_توکی* itemsp)
        : ASTGEN_SUPER_Cover_Trans_سېټ(fl) {
        جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_سېټ;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstCoverWith final : عام Ast_غوټه {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
عام:
    AstCoverWith(File_کرښه* fl, Ast_غوټه* subp, Ast_غوټه_بیان* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        جمع_کړه_Subp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : عام Ast_غوټه {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

عام:
    AstCovergroup(File_کرښه* fl, ثابت string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_غوټه* membersp, Ast_Sen_ونه* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        جمع_کړه_Argsp(argsp);
        جمع_کړه_Sample_Argsp(sampleArgsp);
        جمع_کړه_Membersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ name(ثابت string& name) override { m_name = name; }
    bool maybe_Pointed_منزل() ثابت override { أرجع true; }
};
class AstCoverpointRef final : عام Ast_غوټه {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ثابت string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

عام:
    AstCoverpointRef(File_کرښه* fl, ثابت string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_کرښه* fl, Ast_غوټه_بیان* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstDefParam final : عام Ast_غوټه {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
عام:
    AstDefParam(File_کرښه* fl, Ast_غوټه_بیان* pathp, ثابت string& name, Ast_غوټه_بیان* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_غوټه(ثابت Ast_غوټه*) ثابت override { أرجع true; }
};
class AstDefaultClocking final : عام Ast_غوټه {
    std::string m_name;  // Clocking block name

عام:
    AstDefaultClocking(File_کرښه* fl, ثابت std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstDefaultDisable final : عام Ast_غوټه {
    // @astgen op1 := condp : AstNodeExpr

عام:
    AstDefaultDisable(File_کرښه* fl, Ast_غوټه_بیان* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : عام Ast_غوټه {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
عام:
    AstDpiExport(File_کرښه* fl, ثابت string& vname, ثابت string& cname)
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
class AstElabDisplay final : عام Ast_غوټه {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

عام:
    مضمن AstElabDisplay(File_کرښه* fl, VDisplayType dispType, Ast_غوټه_بیان* exprsp);
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
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        أرجع displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    صحيح instr_شمېر() ثابت override { أرجع INSTR_شمېر_PLI; }
    VDisplayType displayType() ثابت { أرجع m_displayType; }
    فراغ displayType(VDisplayType type) { m_displayType = type; }
};
class Ast_تشه final : عام Ast_غوټه {
    // Represents something missing, e.g. a missing argument in FOREACH
عام:
    explicit Ast_تشه(File_کرښه* fl)
        : ASTGEN_SUPER_تشه(fl) {}
    ASTGEN_MEMBERS_Ast_تشه;
    bool same_غوټه(ثابت Ast_غوټه* /*samep*/) ثابت override { أرجع true; }
};
class Ast_Gen_Case_توکی final : عام Ast_غوټه {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
عام:
    Ast_Gen_Case_توکی(File_کرښه* fl, Ast_غوټه_بیان* condsp, Ast_غوټه* itemsp)
        : ASTGEN_SUPER_Gen_Case_توکی(fl) {
        جمع_کړه_Condsp(condsp);
        جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_Case_توکی;
    bool isDefault() ثابت { أرجع condsp() == nullptr; }
};
class AstImplicit final : عام Ast_غوټه {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
عام:
    AstImplicit(File_کرښه* fl, Ast_غوټه* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        جمع_کړه_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : عام Ast_غوټه {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_پلار_Name;  // Hierarchical path of the declaring module, before inlining
    string m_بنسټ_Name;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
عام:
    AstIntfRef(File_کرښه* fl, ثابت string& name, ثابت string& پلار_Name, ثابت string& بنسټ_Name,
               ثابت string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_پلار_Name{پلار_Name}
        , m_بنسټ_Name{بنسټ_Name}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    string پلار_Name() ثابت { أرجع m_پلار_Name; }
    string بنسټ_Name() ثابت { أرجع m_بنسټ_Name; }
    string modportName() ثابت { أرجع m_modportName; }
};
class AstLibrary final : عام Ast_غوټه {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

عام:
    AstLibrary(File_کرښه* fl, ثابت std::string& name, Ast_غوټه_بیان* filesp, Ast_غوټه_بیان* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        جمع_کړه_Filesp(filesp);
        جمع_کړه_Incdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};
class AstModport final : عام Ast_غوټه {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
عام:
    AstModport(File_کرښه* fl, ثابت string& name, Ast_غوټه* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        جمع_کړه_Varsp(varsp);
    }
    string verilogKwd() ثابت override { أرجع "modport"; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : عام Ast_غوټه {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
عام:
    AstModportClockingRef(File_کرښه* fl, ثابت string& name)
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
class AstModportFTaskRef final : عام Ast_غوټه {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
عام:
    AstModportFTaskRef(File_کرښه* fl, ثابت string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool isImport() ثابت { أرجع !m_export; }
    bool isExport() ثابت { أرجع m_export; }
    Ast_غوټه_FTask* ftaskp() ثابت { أرجع m_ftaskp; }  // [After Link] Pointer to variable
    فراغ ftaskp(Ast_غوټه_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : عام Ast_غوټه {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ثابت VDirection m_direction;  // Direction of the variable (in/out)
عام:
    AstModportVarRef(File_کرښه* fl, ثابت string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_کرښه* fl, ثابت string& name, Ast_غوټه_بیان* exprp,
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
class AstNetlist final : عام Ast_غوټه {
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
    std::string m_resolved_سر_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::سېټ<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ثابت AstConst*, string> m_const_Orig_Param_نومونه;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ثابت Ast_غوټه*, ثابت Ast_غوټه_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::ENUM_پای_ټکی> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::ENUM_پای_ټکی> m_dumpTriggersFuncps{};

عام:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ثابت حرف* broken() ثابت override;
    فراغ ټېل_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.ورننباسه(varp); }
    ثابت std::سېټ<AstVar*>& deferredParamVarps() ثابت { أرجع m_deferredParamVarps; }
    فراغ clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    فراغ deleteContents();
    فراغ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ثابت override VL_MT_STABLE { أرجع "$root"; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    Ast_غوټه_Module* سر_Modulep() ثابت VL_MT_STABLE {  // Top module in hierarchy
        أرجع modulesp();  // First one in the list, for now
    }
    Ast_Type_جدول* typeTablep() { أرجع m_typeTablep; }
    string astConstOrigParamName(ثابت AstConst* nodep) ثابت;
    فراغ astConstOrigParamName(ثابت AstConst* nodep, ثابت string& name);
    فراغ astConstOrigParamNameErase(ثابت AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ثابت Ast_غوټه_Module* containingModule(ثابت Ast_غوټه* nodep);
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
    AstFuncRef* stdPackageProcessSelfp(File_کرښه*) ثابت;
    Ast_سر_Scope* سر_Scopep() ثابت { أرجع m_سر_Scopep; }
    فراغ create_سر_Scope(AstScope* scopep);
    VTimescale timeunit() ثابت { أرجع m_timeunit; }
    فراغ timeunit(ثابت VTimescale& ارزښت) { m_timeunit = ارزښت; }
    VTimescale timeprecision() ثابت { أرجع m_timeprecision; }
    فراغ وخت_Init() {
        m_timeunit = v3Global.opt.وخت_Default_Unit();
        m_timeprecision = v3Global.opt.وخت_Default_Prec();
    }
    فراغ timeprecision_یوځای(File_کرښه*, ثابت VTimescale& ارزښت);
    فراغ timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ثابت { أرجع m_timescaleSpecified; }
    ثابت std::string& resolved_سر_Module_Name() ثابت { أرجع m_resolved_سر_Module_Name; }
    فراغ resolved_سر_Module_Name(ثابت std::string& ارزښت) { m_resolved_سر_Module_Name = ارزښت; }
    uint32_t nTraceCodes() ثابت { أرجع m_nTraceCodes; }
    فراغ nTraceCodes(uint32_t ارزښت) { m_nTraceCodes = ارزښت; }
    AstVarScope* stl_لومړی_Iterationp();
    فراغ clear_Stl_لومړی_Iterationp() { m_stl_لومړی_Iterationp = nullptr; }
    ثابت std::string trace_Lib_سر_Name() ثابت {
        ثابت std::string& name = resolved_سر_Module_Name();
        أرجع prettyName(name.تشه() ? v3Global.rootp()->سر_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    فراغ جمع_کړه_Eval_Stats(ثابت std::string& phase);
};
class AstPackageExport final : عام Ast_غوټه {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

عام:
    AstPackageExport(File_کرښه* fl, ثابت string& pkgName, ثابت string& name)
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
class Ast_Package_Export_ستوری_ستوری final : عام Ast_غوټه {
    // A package export *::* declaration
عام:
    // cppcheck-suppress noExplicitConstructor
    Ast_Package_Export_ستوری_ستوری(File_کرښه* fl)
        : ASTGEN_SUPER_Package_Export_ستوری_ستوری(fl) {}
    ASTGEN_MEMBERS_Ast_Package_Export_ستوری_ستوری;
};
class AstPackageImport final : عام Ast_غوټه {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

عام:
    AstPackageImport(File_کرښه* fl, AstPackage* packagep, ثابت string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_کرښه* fl, ثابت string& pkgName, ثابت string& name)
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
class AstPin final : عام Ast_غوټه {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    صحيح m_pin_عدد;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_param_لار;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
عام:
    AstPin(File_کرښه* fl, صحيح pin_عدد, ثابت string& name, Ast_غوټه* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_عدد{pin_عدد}
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
    bool dot_ستوری() ثابت { أرجع name() == ".*"; }  // Fake name for .* connections until linked
    صحيح pin_عدد() ثابت { أرجع m_pin_عدد; }
    AstVar* modVarp() ثابت { أرجع m_modVarp; }  // [After Link] Pointer to variable
    فراغ modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ثابت { أرجع m_modPTypep; }
    فراغ modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ثابت { أرجع m_param; }
    فراغ param(bool flag) { m_param = flag; }
    ثابت string& param_لار() ثابت { أرجع m_param_لار; }
    فراغ param_لار(ثابت string& لار) { m_param_لار = لار; }
    bool svDotName() ثابت { أرجع m_svDotName; }
    فراغ svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ثابت { أرجع m_svImplicit; }
    فراغ svImplicit(bool flag) { m_svImplicit = flag; }
};
class Ast_پورټ final : عام Ast_غوټه {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ثابت صحيح m_pin_عدد;  // Pin number
    ثابت string m_name;  // Name of pin
عام:
    Ast_پورټ(File_کرښه* fl, صحيح pinnum, ثابت string& name)
        : ASTGEN_SUPER_پورټ(fl)
        , m_pin_عدد{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_پورټ;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Port name
    صحيح pin_عدد() ثابت { أرجع m_pin_عدد; }  // * = Pin number, for order based instantiation
};
class AstPragma final : عام Ast_غوټه {
    ثابت VPragmaType m_pragType;  // Type of pragma
    ثابت VTimescale m_timescale;  // For TIMEUNIT_SET
عام:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_کرښه* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_سېټ, fl, "Should use other constructor");
    }
    AstPragma(File_کرښه* fl, ثابت VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_سېټ}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VPragmaType pragType() ثابت { أرجع m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ثابت override { أرجع false; }
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        أرجع pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ثابت { أرجع m_timescale; }
};
class AstPropSpec final : عام Ast_غوټه {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

عام:
    AstPropSpec(File_کرښه* fl, Ast_Sen_توکی* sensesp, Ast_غوټه_بیان* disablep, Ast_غوټه* propp,
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
    bool hasDType() ثابت override VL_MT_خوندي {
        أرجع true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ثابت { أرجع m_propStrength; }
};
class AstPull final : عام Ast_غوټه {
    // @astgen op1 := lhsp : AstNodeExpr

    ثابت bool m_direction;

عام:
    AstPull(File_کرښه* fl, Ast_غوټه_بیان* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        أرجع direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ثابت { أرجع m_direction; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstScope final : عام Ast_غوټه {
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
    AstScope(File_کرښه* fl, Ast_غوټه_Module* modp, ثابت string& name, AstScope* aboveScopep,
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
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Scope name
    فراغ name(ثابت string& name) override { m_name = name; }
    string pretty_Name_پیغام() ثابت override;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override;
    string nameDotless() ثابت;
    Ast_غوټه_Module* modp() ثابت { أرجع m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    Ast_فعال* comboActivep(bool create);
    //
    AstScope* aboveScopep() ثابت VL_MT_خوندي { أرجع m_aboveScopep; }
    فراغ aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ثابت { أرجع m_aboveCellp; }
    فراغ aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_سر() ثابت VL_MT_خوندي { أرجع aboveScopep() == nullptr; }  // At top of hierarchy
};
class Ast_Sen_توکی final : عام Ast_غوټه {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_څنډه_Type;  // Edge type
عام:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    Ast_Sen_توکی(File_کرښه* fl, VEdgeType څنډه_Type, Ast_غوټه_بیان* senp, Ast_غوټه_بیان* condp = nullptr)
        : ASTGEN_SUPER_Sen_توکی(fl)
        , m_څنډه_Type{څنډه_Type} {
        this->sensp(senp);
        this->condp(condp);
    }
    Ast_Sen_توکی(File_کرښه* fl, Combo)
        : ASTGEN_SUPER_Sen_توکی(fl)
        , m_څنډه_Type{VEdgeType::ET_COMBO} {}
    Ast_Sen_توکی(File_کرښه* fl, Static)
        : ASTGEN_SUPER_Sen_توکی(fl)
        , m_څنډه_Type{VEdgeType::ET_STATIC} {}
    Ast_Sen_توکی(File_کرښه* fl, Initial)
        : ASTGEN_SUPER_Sen_توکی(fl)
        , m_څنډه_Type{VEdgeType::ET_INITIAL} {}
    Ast_Sen_توکی(File_کرښه* fl, InitialNBA)
        : ASTGEN_SUPER_Sen_توکی(fl)
        , m_څنډه_Type{VEdgeType::ET_INITIAL_NBA} {}
    Ast_Sen_توکی(File_کرښه* fl, Final)
        : ASTGEN_SUPER_Sen_توکی(fl)
        , m_څنډه_Type{VEdgeType::ET_FINAL} {}
    Ast_Sen_توکی(File_کرښه* fl, Never)
        : ASTGEN_SUPER_Sen_توکی(fl)
        , m_څنډه_Type{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_Ast_Sen_توکی;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        أرجع څنډه_Type() == VN_DBG_AS(samep, Sen_توکی)->څنډه_Type();
    }
    VEdgeType څنډه_Type() ثابت { أرجع m_څنډه_Type; }
    فراغ څنډه_Type(VEdgeType type) {
        m_څنډه_Type = type;
        edit_شمېر_Inc();
    }
    Ast_غوټه_Var_Ref* varrefp() ثابت { أرجع VN_CAST(sensp(), غوټه_Var_Ref); }
    //
    bool isClocked() ثابت { أرجع څنډه_Type().clockedStmt(); }
    bool is_Combo_Or_ستوری() ثابت {
        أرجع څنډه_Type() == VEdgeType::ET_COMBO || څنډه_Type() == VEdgeType::ET_COMBO_ستوری;
    }
    bool is_Combo_ستوری() ثابت { أرجع څنډه_Type() == VEdgeType::ET_COMBO_ستوری; }
    bool isHybrid() ثابت { أرجع څنډه_Type() == VEdgeType::ET_HYBRID; }
    bool isStatic() ثابت { أرجع څنډه_Type() == VEdgeType::ET_STATIC; }
    bool isInitial() ثابت { أرجع څنډه_Type() == VEdgeType::ET_INITIAL; }
    bool isFinal() ثابت { أرجع څنډه_Type() == VEdgeType::ET_FINAL; }
    bool isNever() ثابت { أرجع څنډه_Type() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_ونه final : عام Ast_غوټه {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
عام:
    Ast_Sen_ونه(File_کرښه* fl, Ast_Sen_توکی* sensesp)
        : ASTGEN_SUPER_Sen_ونه(fl) {
        جمع_کړه_Sensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_ونه;
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    // METHODS
    bool hasClocked() ثابت;  // Includes a clocked statement
    bool has_څنډه() ثابت;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ثابت;  // Includes a STATIC SenItem
    bool hasInitial() ثابت;  // Includes a INITIAL SenItem
    bool hasFinal() ثابت;  // Includes a FINAL SenItem
    bool hasCombo() ثابت;  // Includes a COMBO SenItem
    bool hasHybrid() ثابت;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : عام Ast_غوټه {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

عام:
    AstStrengthSpec(File_کرښه* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { أرجع m_s0; }
    VStrength strength1() { أرجع m_s1; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstSystemCSection final : عام Ast_غوټه {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ثابت VSystemCSectionType m_sectionType;  // The section type
    std::string m_متن;  // The text content

عام:
    AstSystemCSection(File_کرښه* fl, VSystemCSectionType sectionType, ثابت std::string& متن)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_متن{متن} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ثابت { أرجع m_sectionType; }
    ثابت std::string& متن() ثابت { أرجع m_متن; }
    فراغ متن(ثابت std::string& ارزښت) { m_متن = ارزښت; }
    فراغ dump(std::ostream&) ثابت override;
    فراغ dumpJson(std::ostream&) ثابت override;
    bool same_غوټه(ثابت Ast_غوټه*) ثابت override { أرجع false; }
    bool is_خالص() override { أرجع false; }
    bool isOutputter() override { أرجع true; }
};
class Ast_متن final : عام Ast_غوټه {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ثابت std::string m_متن;  // The text to emit
عام:
    Ast_متن(File_کرښه* fl, ثابت std::string& متن)
        : ASTGEN_SUPER_متن(fl)
        , m_متن{متن} {}
    ASTGEN_MEMBERS_Ast_متن;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        أرجع متن() == VN_DBG_AS(samep, متن_2)->متن();
    }
    ثابت std::string& متن() ثابت VL_MT_خوندي { أرجع m_متن; }
};
class Ast_متن_بلاک final : عام Ast_غوټه {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_مختاړی;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ثابت std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
عام:
    explicit Ast_متن_بلاک(File_کرښه* fl,  //
                          ثابت std::string& مختاړی = "",  //
                          ثابت std::string& separator = "",  //
                          ثابت std::string& suffix = "")
        : ASTGEN_SUPER_متن_بلاک(fl)
        , m_مختاړی{مختاړی}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_Ast_متن_بلاک;
    ثابت std::string& مختاړی() ثابت { أرجع m_مختاړی; }
    ثابت std::string& separator() ثابت { أرجع m_separator; }
    ثابت std::string& suffix() ثابت { أرجع m_suffix; }
    // Add some text, or a node to this block
    فراغ جمع_کړه(ثابت string& متن) { جمع_کړه_Nodesp(new Ast_متن{fileline(), متن}); }
    فراغ جمع_کړه(Ast_غوټه* nodep) { جمع_کړه_Nodesp(nodep); }
};
class Ast_سر_Scope final : عام Ast_غوټه {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    ملګری class AstNetlist;  // Only the AstNetlist can create one
    Ast_سر_Scope(File_کرښه* fl, AstScope* ascopep)
        : ASTGEN_SUPER_سر_Scope(fl) {
        this->scopep(ascopep);
    }

عام:
    ASTGEN_MEMBERS_Ast_سر_Scope;
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_لنډمهاله(ثابت string& name, بینښه پلنوالی);
    AstVarScope* create_لنډمهاله(ثابت string& name, Ast_غوټه_DType* dtypep);
};
class Ast_Type_جدول final : عام Ast_غوټه {
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
    using DetailedMap = std::map<VBasic_Type_کلی, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

عام:
    explicit Ast_Type_جدول(File_کرښه* fl);
    ASTGEN_MEMBERS_Ast_Type_جدول;
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    فراغ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* پیدا_کړه_Basic_DType(File_کرښه* fl, VBasicDTypeKwd kwd);
    AstBasicDType* پیدا_کړه_Logic_بټ_DType(File_کرښه* fl, VBasicDTypeKwd kwd, صحيح پلنوالی, صحيح پلنوالی_Min,
                                     VSigning numeric);
    AstBasicDType* پیدا_کړه_Logic_بټ_DType(File_کرښه* fl, VBasicDTypeKwd kwd, ثابت VNumRange& range,
                                     صحيح پلنوالی_Min, VSigning numeric);
    AstBasicDType* پیدا_کړه_Create_Same_DType(AstBasicDType& غوټه);
    AstBasicDType* پیدا_کړه_ورننباسه_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* پیدا_کړه_Constraint_Ref_DType(File_کرښه* fl);
    Ast_تشه_کتار_DType* پیدا_کړه_تشه_کتار_DType(File_کرښه* fl);
    Ast_کتار_DType* پیدا_کړه_کتار_شاخص_DType(File_کرښه* fl);
    Ast_بهیر_DType* پیدا_کړه_بهیر_DType(File_کرښه* fl);
    AstVoidDType* پیدا_کړه_Void_DType(File_کرښه* fl);
    فراغ clearCache();
    فراغ repairCache();
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstTypedef final : عام Ast_غوټه {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_نښان;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_عدد;  // Declaration token number
    bool m_attr_عامه = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

عام:
    AstTypedef(File_کرښه* fl, ثابت string& name, Ast_غوټه* attrsp, VFlag_ماشوم_DType,
               Ast_غوټه_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_عدد{fl->token_عدد()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        ماشوم_DTypep(dtp);  // Only for parser
        جمع_کړه_Attrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_کرښه* fl, ثابت string& name, Ast_غوټه_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_عدد{fl->token_عدد()}
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
    Ast_غوټه_DType* get_ماشوم_DTypep() ثابت override { أرجع ماشوم_DTypep(); }
    مجازي Ast_غوټه_DType* subDTypep() ثابت VL_MT_STABLE {
        أرجع dtypep() ? dtypep() : ماشوم_DTypep();
    }
    // METHODS
    uint32_t decl_Token_عدد() ثابت override { أرجع m_decl_Token_عدد; }
    فراغ decl_Token_عدد_سېټ_Min(uint32_t token_عدد) override {
        m_decl_Token_عدد = std::min(m_decl_Token_عدد, token_عدد);
    }
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    فراغ name(ثابت string& flag) override { m_name = flag; }
    bool attr_عامه() ثابت { أرجع m_attr_عامه; }
    فراغ attr_عامه(bool flag) { m_attr_عامه = flag; }
    bool isHideLocal() ثابت { أرجع m_isHideLocal; }
    فراغ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ثابت { أرجع m_isHideProtected; }
    فراغ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ثابت { أرجع m_isUnderClass; }
    فراغ isUnderClass(bool flag) { m_isUnderClass = flag; }
    فراغ نښان(ثابت string& متن) override { m_نښان = متن; }
    string نښان() ثابت override { أرجع m_نښان; }
};
class AstTypedefFwd final : عام Ast_غوټه {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ثابت VFwdType m_fwdType;  // Forward type for lint check

عام:
    AstTypedefFwd(File_کرښه* fl, ثابت string& name, ثابت VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    VFwdType fwdType() ثابت { أرجع m_fwdType; }
};
class Ast_Udp_جدول final : عام Ast_غوټه {
    // @astgen op1 := linesp : List[AstUdpTableLine]
عام:
    Ast_Udp_جدول(File_کرښه* fl, Ast_Udp_جدول_کرښه* linesp)
        : ASTGEN_SUPER_Udp_جدول(fl) {
        this->جمع_کړه_Linesp(linesp);
        إذا (!v3Global.has_جدول()) v3Global.سېټ_Has_جدول();
    }
    ASTGEN_MEMBERS_Ast_Udp_جدول;
};
class Ast_Udp_جدول_کرښه final : عام Ast_غوټه {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
خاص:
    ثابت bool m_udpIsCombo;  // Combinational or sequential UDP

عام:
    class UdpCombo {};
    Ast_Udp_جدول_کرښه(UdpCombo, File_کرښه* fl, Ast_Udp_جدول_کرښه_Val* iFieldsp,
                    Ast_Udp_جدول_کرښه_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_جدول_کرښه(fl)
        , m_udpIsCombo{true} {
        جمع_کړه_IFieldsp(iFieldsp);
        جمع_کړه_OFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_جدول_کرښه(UdpSequential, File_کرښه* fl, Ast_Udp_جدول_کرښه_Val* iFieldsp,
                    Ast_Udp_جدول_کرښه_Val* oFieldsp1, Ast_Udp_جدول_کرښه_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_جدول_کرښه(fl)
        , m_udpIsCombo{false} {
        جمع_کړه_IFieldsp(iFieldsp);
        جمع_کړه_OFieldsp(oFieldsp1);
        جمع_کړه_OFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_جدول_کرښه;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح udpIsCombo() ثابت { أرجع m_udpIsCombo; }
};
class Ast_Udp_جدول_کرښه_Val final : عام Ast_غوټه {
    string m_متن;  // Value character

عام:
    Ast_Udp_جدول_کرښه_Val(File_کرښه* fl, ثابت string& متن)
        : ASTGEN_SUPER_Udp_جدول_کرښه_Val(fl)
        , m_متن{متن} {}
    ASTGEN_MEMBERS_Ast_Udp_جدول_کرښه_Val;
    string name() ثابت override VL_MT_STABLE { أرجع m_متن; }
    فراغ name(std::string ثابت& متن) override VL_MT_STABLE { m_متن = متن; }
    string متن() ثابت VL_MT_خوندي { أرجع m_متن; }
};
class AstVar final : عام Ast_غوټه {
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
    string m_نښان;  // Holds the string of the verilator tag -- used in JSON output.
    VVarType m_varType;  // Type of variable
    VDirection m_direction;  // Direction input/output etc
    VDirection m_declDirection;  // Declared direction input/output etc
    VLifetime m_lifetime;  // Lifetime
    VRandAttr m_rand;  // Randomizability of this variable (rand, randc, etc)
    صحيح m_pin_عدد = 0;  // For JSON, if non-zero the connection pin number
    صحيح m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
    bool m_libInoutEnable : 1;  // Port is the lowered library inout's enable
    bool m_ansi : 1;  // Params or pins declared in the module header, rather than the body
    bool m_declTyped : 1;  // Declared as type (for dedup check)
    bool m_tristate : 1;  // Inout or triwire or trireg
    bool m_primaryIO : 1;  // In/out to top level (or directly assigned from same)
    bool m_primaryClock : 1;  // In/out to top level and is, or combinationally drives a SenItem
    bool m_sc : 1;  // SystemC variable
    bool m_scSensitive : 1;  // SystemC sensitive() needed
    bool m_sig_عامه : 1;  // User C code accesses this signal or is top signal
    bool m_sig_Mod_عامه : 1;  // User C code accesses this signal and module
    bool m_sig_User_Rd_عامه : 1;  // User C code accesses this signal, read only
    bool m_sigUserRWPublic : 1;  // User C code accesses this signal, read-write
    bool m_کارول_شوی_Param : 1;  // Parameter is referenced (on link; later signals not setup)
    bool m_کارول_شوی_Loop_Idx : 1;  // Variable subject of for unrolling
    bool m_funcLocal : 1;  // Local variable for a function
    bool m_funcLocalSticky : 1;  // As m_funcLocal but remains set if var is moved to a static
    bool m_funcReturn : 1;  // Return variable for a function
    bool m_attrScBv : 1;  // User force bit vector attribute
    bool m_attrScBigUint : 1;  // User force sc_biguint attribute
    bool m_attrSFormat : 1;  // User sformat attribute
    bool m_attr_وېشل_Var : 1;  // declared with split_var metacomment
    bool m_attrFsmState : 1;  // declared with fsm_state metacomment
    bool m_attrFsmArcInclCond : 1;  // declared with fsm_arc_include_cond metacomment
    VFsmExpandType::en m_attrFsmStateExpand : 2;  // FSM expansion type for FSM coverage
    bool m_const_Pool_ثبت : 1;  // Constant pool variable
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
    bool m_is_Iface_پلار : 1;  // dtype is reference to interface present in this module
    bool m_is_Iface_Array_وېشل : 1;  // Interface array that was split into element variables by
                                   // V3Param, until V3Width removes it
    bool m_is_دننی : 1;  // Internal state, don't add to method pinter
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
    bool m_is_Forced_By_کوډ : 1;  // May be forced/released from AstAssignForce/AstRelease
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
    bool m_process_کتار : 1;  // Process queue variable
    bool m_mtask_Cache_کرښه_Align : 1;  // Start MTask affinity group on a cache line
    فراغ init() {
        m_libInoutEnable = false;
        m_ansi = false;
        m_declTyped = false;
        m_tristate = false;
        m_primaryIO = false;
        m_primaryClock = false;
        m_sc = false;
        m_scSensitive = false;
        m_کارول_شوی_Param = false;
        m_کارول_شوی_Loop_Idx = false;
        m_sig_عامه = false;
        m_sig_Mod_عامه = false;
        m_sig_User_Rd_عامه = false;
        m_sigUserRWPublic = false;
        m_funcLocal = false;
        m_funcLocalSticky = false;
        m_funcReturn = false;
        m_attrScBv = false;
        m_attrScBigUint = false;
        m_attrSFormat = false;
        m_attr_وېشل_Var = false;
        m_attrFsmState = false;
        m_attrFsmArcInclCond = false;
        m_attrFsmStateExpand = VFsmExpandType::DEFAULT;
        m_const_Pool_ثبت = false;
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
        m_is_Iface_پلار = false;
        m_is_Iface_Array_وېشل = false;
        m_is_دننی = false;
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
        m_is_Forced_By_کوډ = false;
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
        m_process_کتار = false;
        m_mtask_Cache_کرښه_Align = false;
    }

عام:
    AstVar(File_کرښه* fl, VVarType type, ثابت string& name, VFlag_ماشوم_DType, Ast_غوټه_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ماشوم_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_کرښه* fl, VVarType type, ثابت string& name, Ast_غوټه_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_کرښه* fl, VVarType type, ثابت string& name, VFlagLogicPacked, صحيح wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_سېټ_Logic_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_کرښه* fl, VVarType type, ثابت string& name, VFlag_بټ_Packed, صحيح wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_سېټ_بټ_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_کرښه* fl, VVarType type, ثابت string& name, ثابت AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        إذا (examplep->ماشوم_DTypep()) ماشوم_DTypep(examplep->ماشوم_DTypep()->clone_ونه(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    مضمن bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override;
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }  // * = Var name
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    string origName() ثابت override { أرجع m_origName; }  // * = Original name
    فراغ origName(ثابت string& name) { m_origName = name; }
    VVarType varType() ثابت VL_MT_خوندي { أرجع m_varType; }  // * = Type of variable
    فراغ direction(ثابت VDirection& flag) {
        m_direction = flag;
        إذا (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ثابت VL_MT_خوندي { أرجع m_direction; }
    bool isIO() ثابت VL_MT_خوندي { أرجع m_direction != VDirection::NONE; }
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
    فراغ var_Type2_بهر() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    فراغ var_Type2_دننه() {
        m_tristate = false;
        m_direction = VDirection::ننوت;
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
    Ast_غوټه_DType* get_ماشوم_DTypep() ثابت override { أرجع ماشوم_DTypep(); }
    Ast_غوټه_DType* dtypeSkipRefp() ثابت VL_MT_STABLE { أرجع subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ثابت VL_MT_STABLE { أرجع subDTypep()->basicp(); }
    مجازي Ast_غوټه_DType* subDTypep() ثابت VL_MT_STABLE {
        أرجع dtypep() ? dtypep() : ماشوم_DTypep();
    }
    فراغ ansi(bool flag) { m_ansi = flag; }
    فراغ declTyped(bool flag) { m_declTyped = flag; }
    فراغ sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    فراغ attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    فراغ attrScBv(bool flag) { m_attrScBv = flag; }
    فراغ attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    فراغ attrSFormat(bool flag) { m_attrSFormat = flag; }
    فراغ attr_وېشل_Var(bool flag) { m_attr_وېشل_Var = flag; }
    فراغ attrFsmState(bool flag) { m_attrFsmState = flag; }
    فراغ attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    فراغ attrFsmStateExpand(ثابت VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool const_Pool_ثبت() ثابت { أرجع m_const_Pool_ثبت; }
    فراغ سېټ_Const_Pool_ثبت() { m_const_Pool_ثبت = true; }
    bool covergroupRefMember() ثابت { أرجع m_covergroupRefMember; }
    فراغ covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ثابت { أرجع m_embeddedCovergroup; }
    فراغ embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    فراغ rand(ثابت VRandAttr flag) { m_rand = flag; }
    فراغ کارول_شوی_Param(bool flag) { m_کارول_شوی_Param = flag; }
    فراغ کارول_شوی_Loop_Idx(bool flag) { m_کارول_شوی_Loop_Idx = flag; }
    فراغ sig_عامه(bool flag) { m_sig_عامه = flag; }
    فراغ sig_Mod_عامه(bool flag) { m_sig_Mod_عامه = flag; }
    فراغ sig_User_Rd_عامه(bool flag) {
        m_sig_User_Rd_عامه = flag;
        إذا (flag) sig_عامه(true);
    }
    فراغ sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        إذا (flag) sig_User_Rd_عامه(true);
    }
    فراغ sc(bool flag) { m_sc = flag; }
    فراغ scSensitive(bool flag) { m_scSensitive = flag; }
    فراغ primaryIO(bool flag) { m_primaryIO = flag; }
    فراغ isConst(bool flag) { m_isConst = flag; }
    فراغ isContinuously(bool flag) { m_isContinuously = flag; }
    فراغ is_Iface_پلار(bool flag) { m_is_Iface_پلار = flag; }
    فراغ is_Iface_Array_وېشل(bool flag) { m_is_Iface_Array_وېشل = flag; }
    فراغ is_دننی(bool flag) { m_is_دننی = flag; }
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
    bool isDpiOpenArray() ثابت VL_MT_خوندي { أرجع m_isDpiOpenArray; }
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
    bool process_کتار() ثابت { أرجع m_process_کتار; }
    فراغ process_کتار(bool flag) { m_process_کتار = flag; }
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
    فراغ سېټ_Forceable() { m_isForceable = true; }
    فراغ سېټ_Forced_By_کوډ() { m_is_Forced_By_کوډ = true; }
    bool isForced() ثابت { أرجع m_isForceable || m_is_Forced_By_کوډ; }
    bool isReadByDpi() ثابت { أرجع m_isReadByDpi; }
    فراغ سېټ_Read_By_Dpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ثابت { أرجع m_isWrittenByDpi; }
    فراغ سېټ_Written_By_Dpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ثابت { أرجع m_isWrittenBySuspendable; }
    فراغ سېټ_Written_By_Suspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ثابت { أرجع m_ignorePostRead; }
    فراغ سېټ_Ignore_Post_Read() { m_ignorePostRead = true; }
    bool ignorePostWrite() ثابت { أرجع m_ignorePostWrite; }
    فراغ سېټ_Ignore_Post_Write() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ثابت { أرجع m_ignoreSchedWrite; }
    فراغ سېټ_Ignore_Sched_Write() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ثابت { أرجع m_dfgTriLowered; }
    فراغ سېټ_Dfg_Tri_Lowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ثابت { أرجع m_dfgAllowMultidriveTri; }
    فراغ سېټ_Dfg_Allow_Multidrive_Tri() { m_dfgAllowMultidriveTri = true; }
    فراغ globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ثابت { أرجع m_globalConstrained; }
    bool isStdRandomizeArg() ثابت { أرجع m_isStdRandomizeArg; }
    فراغ سېټ_Std_Randomize_Arg() { m_isStdRandomizeArg = true; }
    // METHODS
    فراغ name(ثابت string& name) override { m_name = name; }
    فراغ نښان(ثابت string& متن) override { m_نښان = متن; }
    string نښان() ثابت override { أرجع m_نښان; }
    bool isAnsi() ثابت { أرجع m_ansi; }
    bool isContinuously() ثابت { أرجع m_isContinuously; }
    bool isDeclTyped() ثابت { أرجع m_declTyped; }
    bool isInout() ثابت { أرجع m_direction.isInout(); }
    bool isInoutOrRef() ثابت { أرجع m_direction.isInoutOrRef(); }
    bool is_ننوت() ثابت { أرجع m_direction.is_ننوت(); }
    bool isNonOutput() ثابت { أرجع m_direction.isNonOutput(); }
    bool isReadOnly() ثابت VL_MT_خوندي { أرجع m_direction.isReadOnly(); }
    bool isConstRef() ثابت VL_MT_خوندي { أرجع m_direction.isConstRef(); }
    bool isRef() ثابت VL_MT_خوندي { أرجع m_direction.isRef(); }
    bool isWritable() ثابت VL_MT_خوندي { أرجع m_direction.isWritable(); }
    bool isTristate() ثابت { أرجع m_tristate; }
    bool isPrimaryIO() ثابت VL_MT_خوندي { أرجع m_primaryIO; }
    bool isPrimaryInish() ثابت { أرجع isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ثابت VL_MT_خوندي { أرجع m_primaryClock; }
    فراغ سېټ_Primary_Clock() { m_primaryClock = true; }
    bool isIfaceRef() ثابت { أرجع varType() == VVarType::IFACEREF; }
    فراغ سېټ_Iface_Ref() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool is_Iface_پلار() ثابت { أرجع m_is_Iface_پلار; }
    bool is_Iface_Array_وېشل() ثابت { أرجع m_is_Iface_Array_وېشل; }
    bool is_دننی() ثابت { أرجع m_is_دننی; }
    bool isIfaceParam() ثابت { أرجع m_isIfaceParam; }
    bool isSignal() ثابت { أرجع varType().isSignal(); }
    bool isNet() ثابت { أرجع varType().isNet(); }
    bool isWor() ثابت { أرجع varType().isWor(); }
    bool isWiredNet() ثابت { أرجع varType().isWiredNet(); }
    bool is_لنډمهاله() ثابت { أرجع varType().is_لنډمهاله(); }
    bool isClassMember() ثابت { أرجع varType() == VVarType::MEMBER; }
    bool isVirtIface() ثابت {
        إذا (AstIfaceRefDType* ثابت dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            أرجع dtp->isVirtual();
        }
        أرجع false;
    }
    bool is_Statement_لنډمهاله() ثابت { أرجع varType() == VVarType::STMTTEMP; }
    bool isXTemp() ثابت { أرجع varType() == VVarType::XTEMP; }
    bool isParam() ثابت { أرجع varType().isParam(); }
    bool isGParam() ثابت { أرجع varType() == VVarType::GPARAM; }
    bool isGenVar() ثابت { أرجع varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ثابت {
        أرجع isIO() || isSignal() || isClassMember() || is_لنډمهاله() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ثابت {
        أرجع !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool is_بټ_Logic() ثابت {
        ثابت AstBasicDType* ثابت bdtypep = basicp();
        أرجع bdtypep && bdtypep->is_بټ_Logic();
    }
    bool is_کارول_شوی_Param() ثابت { أرجع m_کارول_شوی_Param; }
    bool is_کارول_شوی_Loop_Idx() ثابت { أرجع m_کارول_شوی_Loop_Idx; }
    bool mtask_Cache_کرښه_Align() ثابت { أرجع m_mtask_Cache_کرښه_Align; }
    فراغ mtask_Cache_کرښه_Align(bool flag) { m_mtask_Cache_کرښه_Align = flag; }
    bool isSc() ثابت VL_MT_خوندي { أرجع m_sc; }
    bool isScQuad() ثابت;
    bool isScBv() ثابت VL_MT_STABLE;
    bool isScUint() ثابت;
    bool isScUintBool() ثابت;
    bool isScBigUint() ثابت VL_MT_STABLE;
    bool isScSensitive() ثابت { أرجع m_scSensitive; }
    bool is_Sig_عامه() ثابت;
    bool is_Sig_Mod_عامه() ثابت { أرجع m_sig_Mod_عامه && !isIfaceRef(); }
    bool is_Sig_User_Rd_عامه() ثابت { أرجع m_sig_User_Rd_عامه && !isIfaceRef(); }
    bool isSigUserRWPublic() ثابت { أرجع m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ثابت { أرجع m_trace; }
    bool isRand() ثابت { أرجع m_rand.isRand(); }
    bool isRandC() ثابت { أرجع m_rand.isRandC(); }
    bool isConst() ثابت VL_MT_خوندي { أرجع m_isConst; }
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
    bool attr_وېشل_Var() ثابت { أرجع m_attr_وېشل_Var; }
    bool attrFsmState() ثابت { أرجع m_attrFsmState; }
    bool attrFsmArcInclCond() ثابت { أرجع m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ثابت { أرجع m_attrFsmStateExpand; }
    AstIface* sensIfacep() ثابت { أرجع m_sensIfacep; }
    VRandAttr rand() ثابت { أرجع m_rand; }
    string verilogKwd() ثابت override;
    فراغ lifetime(ثابت VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ثابت { أرجع m_lifetime; }
    فراغ pin_عدد(صحيح id) { m_pin_عدد = id; }
    صحيح pin_عدد() ثابت { أرجع m_pin_عدد; }
    فراغ propagateAttrFrom(ثابت AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        إذا (fromp->attrFileDescr()) attrFileDescr(true);
        إذا (fromp->isContinuously()) isContinuously(true);
    }
    فراغ propagate_وېشل_Attr_From(ثابت AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attr_وېشل_Var(fromp->attr_وېشل_Var());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ثابت حرف* cannot_وېشل_ډول_Reason() ثابت;
    فراغ propagateWrapAttrFrom(ثابت AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    فراغ combineType(ثابت AstVar* otherp);
    bool needsCReset() ثابت {
        أرجع !is_Iface_پلار() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_لنډمهاله()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    ساكن AstVar* scVarRecurse(Ast_غوټه* nodep);
};
class AstVarScope final : عام Ast_غوټه {
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
    bool m_fork_By_ارزښت : 1;  // Set before a fork, whose processes get it by value
عام:
    AstVarScope(File_کرښه* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_ارزښت = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    فراغ cloneRelink() override {
        إذا (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    string name() ثابت override VL_MT_STABLE { أرجع scopep()->name() + "->" + varp()->name(); }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override;
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    AstVar* varp() ثابت VL_MT_STABLE { أرجع m_varp; }  // [After Link] Pointer to variable
    فراغ varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ثابت VL_MT_STABLE { أرجع m_scopep; }  // Pointer to scope it's under
    فراغ scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ثابت { أرجع m_trace; }
    فراغ trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ثابت { أرجع m_optimizeLifePost; }
    فراغ optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_ارزښت() ثابت { أرجع m_fork_By_ارزښت; }
    فراغ fork_By_ارزښت(bool flag) { m_fork_By_ارزښت = flag; }
};

// === AstNodeConsItem ===
class Ast_Cons_Array_توکی final : عام Ast_غوټه_Cons_توکی {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ثابت uint64_t m_شاخص;  // Index of the element, with the array's lo() at 0
عام:
    Ast_Cons_Array_توکی(File_کرښه* fl, uint64_t شاخص, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cons_Array_توکی(fl, valuep)
        , m_شاخص{شاخص} {}
    ASTGEN_MEMBERS_Ast_Cons_Array_توکی;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), Cons_Array_توکی));
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        أرجع m_شاخص == VN_DBG_AS(samep, Cons_Array_توکی)->m_شاخص;
    }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }  // By AstConsArray's map
    uint64_t شاخص() ثابت { أرجع m_شاخص; }
};
class Ast_Cons_Assoc_توکی final : عام Ast_غوټه_Cons_توکی {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
عام:
    Ast_Cons_Assoc_توکی(File_کرښه* fl, Ast_غوټه_بیان* keyp, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cons_Assoc_توکی(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Assoc_توکی;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), Cons_Assoc_توکی));
        أرجع nullptr;
    }
};
class Ast_Cons_Dyn_Array_توکی final : عام Ast_غوټه_Cons_توکی {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
عام:
    Ast_Cons_Dyn_Array_توکی(File_کرښه* fl, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_توکی(fl, valuep) {}
    Ast_Cons_Dyn_Array_توکی(File_کرښه* fl, AstConst* keyp, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_توکی(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Dyn_Array_توکی;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), Cons_Dyn_Array_توکی));
        أرجع nullptr;
    }
};
class Ast_Cons_کتار_توکی final : عام Ast_غوټه_Cons_توکی {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
عام:
    Ast_Cons_کتار_توکی(File_کرښه* fl, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cons_کتار_توکی(fl, valuep) {}
    ASTGEN_MEMBERS_Ast_Cons_کتار_توکی;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), Cons_کتار) && !VN_IS(backp(), Cons_کتار_توکی));
        أرجع nullptr;
    }
};
class Ast_Cons_Struct_توکی final : عام Ast_غوټه_Cons_توکی {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
عام:
    Ast_Cons_Struct_توکی(File_کرښه* fl, AstMemberDType* memberp, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cons_Struct_توکی(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_Ast_Cons_Struct_توکی;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), Cons_Struct_توکی));
        أرجع nullptr;
    }
    فراغ dump(std::ostream& str) ثابت override;
    AstMemberDType* memberp() ثابت { أرجع m_memberp; }
};
class Ast_Cons_Wildcard_توکی final : عام Ast_غوټه_Cons_توکی {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
عام:
    Ast_Cons_Wildcard_توکی(File_کرښه* fl, AstConst* keyp, Ast_غوټه_بیان* valuep)
        : ASTGEN_SUPER_Cons_Wildcard_توکی(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Wildcard_توکی;
    ثابت حرف* broken() ثابت override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), Cons_Wildcard_توکی));
        أرجع nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : عام Ast_غوټه_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsm_منزل;  // FSM destination state label
    string m_fsm_نښان;  // FSM arc kind tag (e.g. reset, reset_include, default)
    صحيح m_دننه_واټن;  // Offset column numbers to uniq-ify IFs
عام:
    AstCoverOtherDecl(File_کرښه* fl, ثابت string& پاڼه, ثابت string& comment,
                      ثابت string& linescov, صحيح دننه_واټن, ثابت string& fsmVar = "",
                      ثابت string& fsmFrom = "", ثابت string& fsm_منزل = "",
                      ثابت string& fsm_نښان = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, پاڼه, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsm_منزل{fsm_منزل}
        , m_fsm_نښان{fsm_نښان}
        , m_دننه_واټن{دننه_واټن} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح دننه_واټن() ثابت { أرجع m_دننه_واټن; }
    صحيح اندازه() ثابت override { أرجع 1; }
    ثابت string& linescov() ثابت { أرجع m_linescov; }
    ثابت string& fsmVar() ثابت { أرجع m_fsmVar; }
    ثابت string& fsmFrom() ثابت { أرجع m_fsmFrom; }
    ثابت string& fsm_منزل() ثابت { أرجع m_fsm_منزل; }
    ثابت string& fsm_نښان() ثابت { أرجع m_fsm_نښان; }
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        ثابت AstCoverOtherDecl* ثابت asamep = VN_DBG_AS(samep, CoverOtherDecl);
        أرجع Super::same_غوټه(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : عام Ast_غوټه_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ثابت VNumRange m_range;  // Packed array range covering each toggle bit
عام:
    AstCoverToggleDecl(File_کرښه* fl, ثابت string& پاڼه, ثابت string& comment,
                       ثابت VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, پاڼه, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    صحيح اندازه() ثابت override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        أرجع 2 * m_range.elements();
    }
    ثابت VNumRange& range() ثابت { أرجع m_range; }
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        ثابت AstCoverToggleDecl* ثابت asamep = VN_DBG_AS(samep, CoverToggleDecl);
        أرجع Super::same_غوټه(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : عام Ast_غوټه_FTask {
    // A function inside a module
عام:
    AstFunc(File_کرښه* fl, ثابت string& name, Ast_غوټه* stmtp, Ast_غوټه* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    Ast_غوټه_FTask* cloneType(ثابت string& name) override {
        أرجع new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "function"; }
};
class AstLet final : عام Ast_غوټه_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
عام:
    AstLet(File_کرښه* fl, ثابت string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    ثابت حرف* broken() ثابت override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_بیان));
        أرجع nullptr;
    }
    Ast_غوټه_FTask* cloneType(ثابت string& name) override { أرجع new AstLet{fileline(), name}; }
    string verilogKwd() ثابت override { أرجع "let"; }
};
class AstProperty final : عام Ast_غوټه_FTask {
    // A property inside a module
عام:
    AstProperty(File_کرښه* fl, ثابت string& name, Ast_غوټه* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    Ast_غوټه_FTask* cloneType(ثابت string& name) override {
        أرجع new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "property"; }
};
class AstSequence final : عام Ast_غوټه_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
عام:
    AstSequence(File_کرښه* fl, ثابت string& name, Ast_غوټه* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool hasDType() ثابت override VL_MT_خوندي { أرجع true; }
    Ast_غوټه_FTask* cloneType(ثابت string& name) override {
        أرجع new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "sequence"; }
    bool isReferenced() ثابت { أرجع m_referenced; }
    فراغ isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : عام Ast_غوټه_FTask {
    // A task inside a module
عام:
    AstTask(File_کرښه* fl, ثابت string& name, Ast_غوټه* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_غوټه_FTask* cloneType(ثابت string& name) override {
        أرجع new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ثابت override { أرجع "task"; }
};

// === AstNodeFile ===
class AstCFile final : عام Ast_غوټه_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_سرچینه : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
عام:
    AstCFile(File_کرښه* fl, ثابت string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_سرچینه{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
    uint64_t complexityScore() ثابت { أرجع m_complexityScore; }
    فراغ complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ثابت { أرجع m_slow; }
    فراغ slow(bool flag) { m_slow = flag; }
    bool سرچینه() ثابت { أرجع m_سرچینه; }
    فراغ سرچینه(bool flag) { m_سرچینه = flag; }
    bool support() ثابت { أرجع m_support; }
    فراغ support(bool flag) VL_MT_خوندي { m_support = flag; }
};
class AstVFile final : عام Ast_غوټه_File {
    // Verilog output file
    // Parents:  NETLIST
عام:
    AstVFile(File_کرښه* fl, ثابت string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    فراغ dump(std::ostream& str = std::cout) ثابت override;
    فراغ dumpJson(std::ostream& str = std::cout) ثابت override;
};
class AstCoverCross final : عام Ast_غوټه_Func_Cov_توکی {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
عام:
    AstCoverCross(File_کرښه* fl, ثابت string& name, AstCoverpointRef* itemsp,
                  Ast_غوټه_بیان* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        جمع_کړه_Itemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class AstCoverpoint final : عام Ast_غوټه_Func_Cov_توکی {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
عام:
    AstCoverpoint(File_کرښه* fl, ثابت string& name, Ast_غوټه_بیان* exprp,
                  Ast_غوټه_بیان* iffp = nullptr, Ast_غوټه* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        جمع_کړه_Binsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};

// === AstNodeGen ===
class Ast_Gen_بلاک final : عام Ast_غوټه_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ثابت bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ثابت bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

عام:
    Ast_Gen_بلاک(File_کرښه* fl, ثابت string& name, Ast_غوټه* itemsp, bool implied)
        : ASTGEN_SUPER_Gen_بلاک(fl)
        , m_name{name}
        , m_unnamed{name.تشه()}
        , m_implied{implied} {
        this->جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_بلاک;
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
class AstGenCase final : عام Ast_غوټه_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
عام:
    AstGenCase(File_کرښه* fl, Ast_غوټه_بیان* exprp, Ast_Gen_Case_توکی* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : عام Ast_غوټه_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
عام:
    AstGenFor(File_کرښه* fl, Ast_غوټه* initsp, Ast_غوټه_بیان* condp, Ast_غوټه* incsp, Ast_غوټه* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->جمع_کړه_Initsp(initsp);
        this->condp(condp);
        this->جمع_کړه_Incsp(incsp);
        this->جمع_کړه_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : عام Ast_غوټه_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
عام:
    AstGenIf(File_کرښه* fl, Ast_غوټه_بیان* condp, Ast_غوټه* thensp, Ast_غوټه* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->جمع_کړه_Thensp(thensp);
        this->جمع_کړه_Elsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : عام Ast_غوټه_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtype_Name_ډک;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_عدد;  // Declaration token number
    VBaseOverride m_بنسټ_Override;  // BaseOverride (inital/final/extends)
    bool m_hasRandVarsUpdate = false;  // Has updateRandVars method,
                                       // which updates pointers to rand variables in clone()
    bool m_covergroup = false;  // Is covergroup (TODO perhaps make a new Ast node type for CG?)
    bool m_extended = false;  // Is extension or extended by other classes
    bool m_interfaceClass = false;  // Interface class
    bool m_needRNG = false;  // Need RNG, uses srandom/randomize
    bool m_use_Virtual_عامه = false;  // Subclasses need virtual public as uses interface class
    bool m_virtual = false;  // Virtual class
    bool m_printedFrom = false;  // This class is printed from i.e. is used as format arg.
    // Mutable, as only a recursion guard of the const dtypeName(), so also not dumped
    mutable bool m_dtypeNameBusy = false;  // In dtypeName(), which a parameter may lead back to
    // Covergroup options (when m_covergroup is true)
    صحيح m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool ډک_2, ثابت string& scope) ثابت;
    string dtypeNameScope() ثابت;  // Prefix of dtypeName(true) for the scope declaring the class

عام:
    AstClass(File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_عدد{fl->token_عدد()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ثابت override { أرجع isCovergroup() ? "covergroup" : "class"; }
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع true; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    bool timescaleMatters() ثابت override { أرجع false; }
    AstClassPackage* classOrPackagep() ثابت VL_MT_STABLE { أرجع m_classOrPackagep; }
    فراغ classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ثابت VL_MT_STABLE {
        أرجع m_covergroupEnclosingClassp;
    }
    فراغ covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_غوټه* membersp() ثابت VL_MT_STABLE { أرجع stmtsp(); }
    فراغ جمع_کړه_Membersp(Ast_غوټه* nodep) { جمع_کړه_Stmtsp(nodep); }
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
    bool use_Virtual_عامه() ثابت { أرجع m_use_Virtual_عامه; }
    فراغ use_Virtual_عامه(bool flag) { m_use_Virtual_عامه = flag; }
    فراغ markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ثابت { أرجع m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool ډک_2) ثابت;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtype_Name_دننه(ثابت string& scope) ثابت;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    فراغ dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ثابت { أرجع !m_dtype_Name_ډک.تشه(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_پیغام() ثابت override { أرجع hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    ساكن string embeddedCovergroupTypeName(ثابت string& name) { أرجع "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ثابت;
    // Covergroup options accessors
    صحيح cgAutoBinMax() ثابت { أرجع m_cgAutoBinMax; }
    فراغ cgAutoBinMax(صحيح ارزښت) { m_cgAutoBinMax = ارزښت; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    ساكن bool isClassExtendedFrom(ثابت AstClass* refClassp, ثابت AstClass* بنسټ_Classp);
    uint32_t decl_Token_عدد() ثابت override { أرجع m_decl_Token_عدد; }
    فراغ decl_Token_عدد_سېټ_Min(uint32_t token_عدد) override {
        m_decl_Token_عدد = std::min(m_decl_Token_عدد, token_عدد);
    }
    فراغ بنسټ_Override(ثابت VBaseOverride& flag) { m_بنسټ_Override = flag; }
    VBaseOverride بنسټ_Override() ثابت { أرجع m_بنسټ_Override; }
    // Return the lowest class extended from, or this class
    AstClass* بنسټ_Most_Classp();
    ساكن bool is_Cacheable_ماشوم(ثابت Ast_غوټه* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    کينډۍ <typename T_Callable>
    فراغ foreachMember(ثابت T_Callable& f) {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_غوټه*>::ارزښت
                && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        إذا (ثابت AstClassExtends* ثابت cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        لكل (Ast_غوټه* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_غوټه::شخصي_Type_Test<T_غوټه>(stmtp)) f(this, static_cast<T_غوټه*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    کينډۍ <typename T_Callable>
    bool existsMember(ثابت T_Callable& p) ثابت {
        using T_غوټه = typename Function_Arg_No_اشاره_ګر_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ثابت AstClass*, ثابت T_غوټه*>::ارزښت
                && std::is_بنسټ_د<Ast_غوټه, T_غوټه>::ارزښت,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        إذا (ثابت AstClassExtends* ثابت cextendsp = this->extendsp()) {
            إذا (cextendsp->classp()->existsMember(p)) أرجع true;
        }
        لكل (Ast_غوټه* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (Ast_غوټه::شخصي_Type_Test<T_غوټه>(stmtp)) {
                إذا (p(this, static_cast<T_غوټه*>(stmtp))) أرجع true;
            }
        }
        أرجع false;
    }
};
class AstClassPackage final : عام Ast_غوټه_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
عام:
    AstClassPackage(File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ثابت override { أرجع "classpackage"; }
    bool timescaleMatters() ثابت override { أرجع false; }
    AstClass* classp() ثابت VL_MT_خوندي { أرجع m_classp; }
    فراغ classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : عام Ast_غوټه_Module {
    // An interface declaration
    string m_dtype_Name_ډک;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
عام:
    AstIface(File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    string verilogKwd() ثابت override { أرجع "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ثابت override { أرجع false; }
    bool hasVirtualRef() ثابت { أرجع m_hasVirtualRef; }
    فراغ سېټ_Has_Virtual_Ref() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool ډک_2) ثابت;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    فراغ dtypeNameFreeze();
};
class AstModule final : عام Ast_غوټه_Module {
    // A module declaration
    ثابت bool m_isChecker;  // Module represents a checker
    ثابت bool m_is_پروګرام;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

عام:
    class Checker {};  // for constructor type-overload selection
    class پروګرام {};  // for constructor type-overload selection
    AstModule(File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_پروګرام{false} {}
    AstModule(File_کرښه* fl, ثابت string& name, ثابت string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_is_پروګرام{false} {}
    AstModule(File_کرښه* fl, ثابت string& name, ثابت string& libname, پروګرام)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_پروګرام{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ثابت override {
        أرجع m_isChecker ? "checker" : m_is_پروګرام ? "program" : "module";
    }
    bool timescaleMatters() ثابت override { أرجع true; }
    bool isChecker() ثابت { أرجع m_isChecker; }
    bool is_پروګرام() ثابت { أرجع m_is_پروګرام; }
    bool hasGenericIface() ثابت { أرجع m_hasGenericIface; }
    فراغ hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
};
class Ast_Not_وموندل_Module final : عام Ast_غوټه_Module {
    // A missing module declaration
عام:
    Ast_Not_وموندل_Module(File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Not_وموندل_Module(fl, name, libname) {}
    ASTGEN_MEMBERS_Ast_Not_وموندل_Module;
    string verilogKwd() ثابت override { أرجع "/*not-found-*/ module"; }
    bool timescaleMatters() ثابت override { أرجع false; }
};
class AstPackage final : عام Ast_غوټه_Module {
    // A package declaration
عام:
    AstPackage(File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ثابت override { أرجع "package"; }
    bool timescaleMatters() ثابت override { أرجع !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : عام Ast_غوټه_Module {
    // A primitive declaration
عام:
    AstPrimitive(File_کرښه* fl, ثابت string& name, ثابت string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ثابت override { أرجع "primitive"; }
    bool timescaleMatters() ثابت override { أرجع false; }
};

// === AstNodeProcedure ===
class AstAlways final : عام Ast_غوټه_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ثابت VAlwaysKwd m_keyword;

عام:
    AstAlways(File_کرښه* fl, VAlwaysKwd keyword, Ast_Sen_ونه* sentreep, Ast_غوټه* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit مضمن AstAlways(Ast_ټاکل_W* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    VAlwaysKwd keyword() ثابت { أرجع m_keyword; }
};
class AstAlwaysObserved final : عام Ast_غوټه_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

عام:
    AstAlwaysObserved(File_کرښه* fl, Ast_Sen_ونه* sentreep, Ast_غوټه* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : عام Ast_غوټه_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

عام:
    explicit AstAlwaysPost(File_کرښه* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : عام Ast_غوټه_Procedure {
    // Like always but Postponed scheduling region

عام:
    AstAlwaysPostponed(File_کرښه* fl, Ast_غوټه* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : عام Ast_غوټه_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

عام:
    explicit AstAlwaysPre(File_کرښه* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : عام Ast_غوټه_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

عام:
    AstAlwaysReactive(File_کرښه* fl, Ast_Sen_ونه* sentreep, Ast_غوټه* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : عام Ast_غوټه_Procedure {
عام:
    AstFinal(File_کرښه* fl, Ast_غوټه* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : عام Ast_غوټه_Procedure {
عام:
    AstInitial(File_کرښه* fl, Ast_غوټه* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : عام Ast_غوټه_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
عام:
    AstInitialAutomatic(File_کرښه* fl, Ast_غوټه* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : عام Ast_غوټه_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
عام:
    AstInitialStatic(File_کرښه* fl, Ast_غوټه* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : عام Ast_غوټه_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
عام:
    AstBracketRange(File_کرښه* fl, Ast_غوټه* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    مجازي string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_غوټه(ثابت Ast_غوټه* /*samep*/) ثابت override { أرجع true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_منزل() ثابت override VL_MT_خوندي { أرجع false; }
};
class AstRange final : عام Ast_غوټه_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ثابت bool m_fromBracket = false;  // From C-style '[x]' declaration
عام:
    AstRange(File_کرښه* fl, Ast_غوټه_بیان* leftp, Ast_غوټه_بیان* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    مضمن AstRange(File_کرښه* fl, صحيح کیڼ_2, صحيح ښی_2);
    مضمن AstRange(File_کرښه* fl, ثابت VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    مضمن صحيح کیڼ_Const() ثابت VL_MT_STABLE;
    مضمن صحيح ښی_Const() ثابت VL_MT_STABLE;
    صحيح hiConst() ثابت VL_MT_STABLE {
        ثابت صحيح l = کیڼ_Const();
        ثابت صحيح r = ښی_Const();
        أرجع l > r ? l : r;
    }
    صحيح loConst() ثابت VL_MT_STABLE {
        ثابت صحيح l = کیڼ_Const();
        ثابت صحيح r = ښی_Const();
        أرجع l > r ? r : l;
    }
    صحيح elementsConst() ثابت VL_MT_STABLE { أرجع hiConst() - loConst() + 1; }
    bool ascending() ثابت { أرجع کیڼ_Const() < ښی_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_کیڼ_منزل_ښی() ثابت {
        أرجع VNumRange{کیڼ_Const(), ښی_Const()}.seq_کیڼ_منزل_ښی();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_ښی_منزل_کیڼ() ثابت {
        أرجع VNumRange{کیڼ_Const(), ښی_Const()}.seq_ښی_منزل_کیڼ();
    }
    فراغ dump(std::ostream& str) ثابت override;
    فراغ dumpJson(std::ostream& str) ثابت override;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_غوټه(ثابت Ast_غوټه* samep) ثابت override {
        ثابت AstRange* ثابت asamep = VN_DBG_AS(samep, Range);
        أرجع fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ثابت { أرجع m_fromBracket; }
};
class AstUnsizedRange final : عام Ast_غوټه_Range {
    // Unsized range specification, for open arrays
عام:
    explicit AstUnsizedRange(File_کرښه* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    مجازي string emitVerilog() { أرجع "[]"; }
    bool same_غوټه(ثابت Ast_غوټه* /*samep*/) ثابت override { أرجع true; }
};
class AstWildcardRange final : عام Ast_غوټه_Range {
    // Wildcard range specification, for wildcard index type associative arrays
عام:
    explicit AstWildcardRange(File_کرښه* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    مجازي string emitC() { V3ERROR_NA_RETURN(""); }
    مجازي string emitVerilog() { أرجع "[*]"; }
    bool same_غوټه(ثابت Ast_غوټه* /*samep*/) ثابت override { أرجع true; }
};

#د_که_پای  // Guard
