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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ASTNODES_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ASTNODES_H_

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3AST_H_
#ਗਲਤੀ "Use V3Ast.h as the include"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#ਜੇ_ਅੰਤ

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_ਗੰਢ_Cons_ਵਸਤੂ VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    ਮਿੱਤਰ class Ast_ਗੰਢ_ਵਿਅੰਜਕ;  // For m_knownConstantTree
    mutable bool m_ਗਿਆਤ_Constant_ਰੁੱਖ = false;  // Known to be a tree of constants

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Cons_ਵਸਤੂ(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : Ast_ਗੰਢ{t, fl} {
        this->valuep(valuep);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Cons_ਵਸਤੂ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class Ast_ਗੰਢ_Cover_Decl VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_ਪੰਨਾ;  // Coverage point's page tag
    string m_ਪਾਠ;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    ਪੂਰਨ m_bin_ਅੰਕ = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    ਪੂਰਨ m_local_Bin_ਅੰਕ = 0;  // Per-module coverage bin offset
    bool m_ਪ੍ਰਤੀ_Instance = false;  // Don't clone during inlining, already per instance
ਜਨਤਕ:
    Ast_ਗੰਢ_Cover_Decl(VNType t, File_ਸਤਰ* fl, ਅਚਲ string& ਪੰਨਾ, ਅਚਲ string& comment)
        : Ast_ਗੰਢ(t, fl)
        , m_ਪੰਨਾ{ਪੰਨਾ}
        , m_ਪਾਠ{comment} {}
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Cover_Decl;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        ਜੇ (m_ਅੰਕੜੇ_Declp
            && (m_ਅੰਕੜੇ_Declp == this || m_ਅੰਕੜੇ_Declp->m_ਅੰਕੜੇ_Declp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvt_ਮੰਜ਼ਿਲ_Hex(this));
        }
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 1 + 2 * INSTR_ਗਿਣਤੀ_LD; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਪੂਰਨ bin_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_bin_ਅੰਕ; }
    ਖਾਲੀ bin_ਅੰਕ(ਪੂਰਨ flag) { m_bin_ਅੰਕ = flag; }
    ਪੂਰਨ local_Bin_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_local_Bin_ਅੰਕ; }
    ਖਾਲੀ local_Bin_ਅੰਕ(ਪੂਰਨ flag) { m_local_Bin_ਅੰਕ = flag; }
    bool ਪ੍ਰਤੀ_Instance() ਅਚਲ { ਵਾਪਸ m_ਪ੍ਰਤੀ_Instance; }
    ਖਾਲੀ ਪ੍ਰਤੀ_Instance(bool flag) { m_ਪ੍ਰਤੀ_Instance = flag; }
    ਆਭਾਸੀ ਪੂਰਨ ਮਾਪ() ਅਚਲ = 0;
    ਅਚਲ string& comment() ਅਚਲ { ਵਾਪਸ m_ਪਾਠ; }  // text to insert in code
    ਅਚਲ string& ਪੰਨਾ() ਅਚਲ { ਵਾਪਸ m_ਪੰਨਾ; }
    ਅਚਲ string& hier() ਅਚਲ { ਵਾਪਸ m_hier; }
    ਖਾਲੀ hier(ਅਚਲ string& flag) { m_hier = flag; }
    ਖਾਲੀ comment(ਅਚਲ string& flag) { m_ਪਾਠ = flag; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ Ast_ਗੰਢ_Cover_Decl* ਅਚਲ asamep = VN_DBG_AS(samep, ਗੰਢ_Cover_Decl);
        ਵਾਪਸ (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && ਪੰਨਾ() == asamep->ਪੰਨਾ()
                && ਪ੍ਰਤੀ_Instance() == asamep->ਪ੍ਰਤੀ_Instance());
    }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    ਖਾਲੀ ਅੰਕੜੇ_Declp(Ast_ਗੰਢ_Cover_Decl* nodep) { m_ਅੰਕੜੇ_Declp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_ਗੰਢ_Cover_Decl* ਅੰਕੜੇ_Decl_Nullp() ਅਚਲ { ਵਾਪਸ m_ਅੰਕੜੇ_Declp; }
    Ast_ਗੰਢ_Cover_Decl* ਅੰਕੜੇ_Decl_Thisp() { ਵਾਪਸ ਅੰਕੜੇ_Decl_Nullp() ? ਅੰਕੜੇ_Decl_Nullp() : this; }
};
class Ast_ਗੰਢ_FTask VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
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
    string m_iface_ਪੋਰਟ_Name;  // Interface port name for out-of-block definition (IEEE 25.8)
    uint64_t m_dpi_Open_ਮਾਪਾ = 0;  // DPI import open array, if !=0, how many callees
    bool m_task_ਜਨਤਕ : 1;  // Public task
    bool m_class_ਵਿਧੀ : 1;  // Class method
    bool m_didProto : 1;  // Did prototype processing
    bool m_prototype : 1;  // Just a prototype
    bool m_dpiExport : 1;  // DPI exported
    bool m_dpiImport : 1;  // DPI imported
    bool m_dpiContext : 1;  // DPI import context
    bool m_dpi_Open_ਬੱਚਾ : 1;  // DPI import open array child wrapper
    bool m_dpiTask : 1;  // DPI import task (vs. void function)
    bool m_isConstructor : 1;  // Class constructor
    bool m_isExternProto : 1;  // Extern prototype
    bool m_isExternDef : 1;  // Extern definition
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_dpi_ਸ਼ੁੱਧ : 1;  // DPI import pure (vs. virtual pure)
    bool m_keepAlive : 1;  // Disable dead function elimination
    bool m_needsSyms : 1;  // Requires vlSymsp argument
    bool m_ਸ਼ੁੱਧ_Virtual : 1;  // Pure virtual
    bool m_recursive : 1;  // Recursive or part of recursion
    bool m_static : 1;  // Static method in class
    bool m_underGenerate : 1;  // Under generate (for warning)
    bool m_verilogFunction : 1;  // Declared by user as function (versus internal-made)
    bool m_verilogTask : 1;  // Declared by user as task (versus internal-made)
    bool m_virtual : 1;  // Virtual method in class
    bool m_needProcess : 1;  // Needs access to VlProcess of the caller
    bool m_isCovergroupSample : 1;  // Covergroup sample() method
    VBaseOverride m_ਅਧਾਰ_Override;  // BaseOverride (inital/final/extends)
    VLifetime m_lifetime;  // Default lifetime of local vars
    VIsCached m_purity;  // Pure state

ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_FTask(VNType t, File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* stmtsp)
        : Ast_ਗੰਢ{t, fl}
        , m_name{name}
        , m_task_ਜਨਤਕ{false}
        , m_class_ਵਿਧੀ{false}
        , m_didProto{false}
        , m_prototype{false}
        , m_dpiExport{false}
        , m_dpiImport{false}
        , m_dpiContext{false}
        , m_dpi_Open_ਬੱਚਾ{false}
        , m_dpiTask{false}
        , m_isConstructor{false}
        , m_isExternProto{false}
        , m_isExternDef{false}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_dpi_ਸ਼ੁੱਧ{false}
        , m_keepAlive{false}
        , m_needsSyms{true}
        , m_ਸ਼ੁੱਧ_Virtual{false}
        , m_recursive{false}
        , m_static{false}
        , m_underGenerate{false}
        , m_verilogFunction{false}
        , m_verilogTask{false}
        , m_virtual{false}
        , m_needProcess{false}
        , m_isCovergroupSample{false} {
        ਜੋੜੋ_Stmtsp(stmtsp);
        cname(name);  // Might be overridden by dpi import/export
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_FTask;
    ਆਭਾਸੀ Ast_ਗੰਢ_FTask* cloneType(ਅਚਲ string& name) = 0;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool isGateOptimizable() ਅਚਲ override {
        ਵਾਪਸ !((m_dpiExport || m_dpiImport) && !m_dpi_ਸ਼ੁੱਧ);
    }
    // {AstFunc only} op1 = Range output variable
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string cname() ਅਚਲ { ਵਾਪਸ m_cname; }
    ਖਾਲੀ cname(ਅਚਲ string& cname) { m_cname = cname; }
    bool isFunction() ਅਚਲ { ਵਾਪਸ fvarp() != nullptr; }
    // MORE ACCESSORS
    ਖਾਲੀ dpi_Open_ਮਾਪਾ_Inc() { ++m_dpi_Open_ਮਾਪਾ; }
    ਖਾਲੀ dpi_Open_ਮਾਪਾ_Clear() { m_dpi_Open_ਮਾਪਾ = 0; }
    uint64_t dpi_Open_ਮਾਪਾ() ਅਚਲ { ਵਾਪਸ m_dpi_Open_ਮਾਪਾ; }
    bool task_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_task_ਜਨਤਕ; }
    ਖਾਲੀ task_ਜਨਤਕ(bool flag) { m_task_ਜਨਤਕ = flag; }
    bool class_ਵਿਧੀ() ਅਚਲ { ਵਾਪਸ m_class_ਵਿਧੀ; }
    ਖਾਲੀ class_ਵਿਧੀ(bool flag) { m_class_ਵਿਧੀ = flag; }
    bool didProto() ਅਚਲ { ਵਾਪਸ m_didProto; }
    ਖਾਲੀ didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ਅਚਲ { ਵਾਪਸ m_isExternProto; }
    ਖਾਲੀ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ਅਚਲ { ਵਾਪਸ m_isExternDef; }
    ਖਾਲੀ isExternDef(bool flag) { m_isExternDef = flag; }
    ਅਚਲ string& iface_ਪੋਰਟ_Name() ਅਚਲ { ਵਾਪਸ m_iface_ਪੋਰਟ_Name; }
    ਖਾਲੀ iface_ਪੋਰਟ_Name(ਅਚਲ string& name) { m_iface_ਪੋਰਟ_Name = name; }
    bool prototype() ਅਚਲ { ਵਾਪਸ m_prototype; }
    ਖਾਲੀ prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ਅਚਲ { ਵਾਪਸ m_dpiExport; }
    ਖਾਲੀ dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ਅਚਲ { ਵਾਪਸ m_dpiImport; }
    ਖਾਲੀ dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ਅਚਲ { ਵਾਪਸ m_dpiContext; }
    ਖਾਲੀ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpi_Open_ਬੱਚਾ() ਅਚਲ { ਵਾਪਸ m_dpi_Open_ਬੱਚਾ; }
    ਖਾਲੀ dpi_Open_ਬੱਚਾ(bool flag) { m_dpi_Open_ਬੱਚਾ = flag; }
    bool dpiTask() ਅਚਲ { ਵਾਪਸ m_dpiTask; }
    ਖਾਲੀ dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ਅਚਲ { ਵਾਪਸ !m_dpiCDecl.ਖਾਲੀ_ਥਾਵਾਂ(); }
    ਅਚਲ string& dpiCDecl() ਅਚਲ { ਵਾਪਸ m_dpiCDecl; }
    ਖਾਲੀ dpiCDecl(ਅਚਲ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ਅਚਲ { ਵਾਪਸ m_isConstructor; }
    ਖਾਲੀ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ਅਚਲ { ਵਾਪਸ m_isHideLocal; }
    ਖਾਲੀ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ਅਚਲ { ਵਾਪਸ m_isHideProtected; }
    ਖਾਲੀ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpi_ਸ਼ੁੱਧ() ਅਚਲ { ਵਾਪਸ m_dpi_ਸ਼ੁੱਧ; }
    ਖਾਲੀ dpi_ਸ਼ੁੱਧ(bool flag) { m_dpi_ਸ਼ੁੱਧ = flag; }
    bool keepAlive() ਅਚਲ { ਵਾਪਸ m_keepAlive; }
    ਖਾਲੀ keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ਅਚਲ { ਵਾਪਸ m_needsSyms; }
    ਖਾਲੀ needsSyms(bool flag) { m_needsSyms = flag; }
    bool ਸ਼ੁੱਧ_Virtual() ਅਚਲ { ਵਾਪਸ m_ਸ਼ੁੱਧ_Virtual; }
    ਖਾਲੀ ਸ਼ੁੱਧ_Virtual(bool flag) { m_ਸ਼ੁੱਧ_Virtual = flag; }
    bool recursive() ਅਚਲ { ਵਾਪਸ m_recursive; }
    ਖਾਲੀ recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ਅਚਲ { ਵਾਪਸ m_static; }
    ਖਾਲੀ isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ਅਚਲ { ਵਾਪਸ m_underGenerate; }
    ਖਾਲੀ underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ਅਚਲ { ਵਾਪਸ m_verilogFunction; }
    ਖਾਲੀ verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ਅਚਲ { ਵਾਪਸ m_verilogTask; }
    ਖਾਲੀ verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ਅਚਲ { ਵਾਪਸ m_virtual; }
    ਖਾਲੀ isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ਅਚਲ { ਵਾਪਸ m_needProcess; }
    ਖਾਲੀ ਸੈੱਟ_Need_Process() { m_needProcess = true; }
    bool isCovergroupSample() ਅਚਲ { ਵਾਪਸ m_isCovergroupSample; }
    ਖਾਲੀ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    ਖਾਲੀ ਅਧਾਰ_Override(ਅਚਲ VBaseOverride& flag) { m_ਅਧਾਰ_Override = flag; }
    VBaseOverride ਅਧਾਰ_Override() ਅਚਲ { ਵਾਪਸ m_ਅਧਾਰ_Override; }
    ਖਾਲੀ lifetime(ਅਚਲ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ਅਚਲ { ਵਾਪਸ m_lifetime; }
    bool is_ਸ਼ੁੱਧ() override;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ propagateAttrFrom(ਅਚਲ Ast_ਗੰਢ_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        class_ਵਿਧੀ(fromp->class_ਵਿਧੀ());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

ਨਿਜੀ:
    bool getPurityRecurse() ਅਚਲ;
};
class Ast_ਗੰਢ_File VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
ਜਨਤਕ:
    Ast_ਗੰਢ_File(VNType t, File_ਸਤਰ* fl, ਅਚਲ string& name)
        : Ast_ਗੰਢ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ਗੰਢ_File;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਗੰਢ_Func_Cov_ਵਸਤੂ VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // Base class for functional coverage items (coverpoints, crosses)
ਸੁਰੱਖਿਅਤ:
    string m_name;  // Item name

ਜਨਤਕ:
    Ast_ਗੰਢ_Func_Cov_ਵਸਤੂ(VNType t, File_ਸਤਰ* fl, ਅਚਲ string& name)
        : Ast_ਗੰਢ{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Func_Cov_ਵਸਤੂ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_ਗੰਢ_Gen VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // Generate construct
ਜਨਤਕ:
    Ast_ਗੰਢ_Gen(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ{t, fl} {}
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Gen;
};
class Ast_ਗੰਢ_Module VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    ਅਚਲ string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    ਪੂਰਨ m_ਡੂੰਘਾਈ = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    ਪੂਰਨ m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
    VLifetime m_lifetime;  // Lifetime
    VTimescale m_timeunit;  // Global time unit
    VOptionBool m_unconnectedDrive;  // State of `unconnected_drive
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

    bool m_mod_ਜਨਤਕ : 1;  // Module has public references
    bool m_modTrace : 1;  // Tracing this module
    bool m_ਵਿੱਚ_Library : 1;  // From a library, no error if not used, never top level
    bool m_ctorVarReset : 1;  // Ctor needs to call ctor_var_reset
    bool m_dead : 1;  // LinkDot believes is dead; will remove in Dead visitors
    bool m_hasGParam : 1;  // Has global parameter (for link)
    bool m_hasParameterList : 1;  // Has #() for parameter declaration
    bool m_hier_ਖੰਡ : 1;  // Hierarchical Block marked by HIER_BLOCK pragma
    bool m_hierParams : 1;  // Block containing params for parameterized hier blocks
    bool m_recursive : 1;  // Recursive module
    bool m_recursiveClone : 1;  // If recursive, what module it clones, otherwise nullptr
    bool m_parameterizedTemplate : 1;  // True when at least one specialized clone exists;
                                       // set by V3Param::deepCloneModule. Suppresses
                                       // width/type errors on the unresolved template.
    bool m_verilatorLib : 1;  // Module is a stub for a Verilator produced --lib-create
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Module(VNType t, File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : Ast_ਗੰਢ{t, fl}
        , m_name{name}
        , m_origName{name}
        , m_libname{libname}
        , m_mod_ਜਨਤਕ{false}
        , m_modTrace{false}
        , m_ਵਿੱਚ_Library{false}
        , m_ctorVarReset{false}
        , m_dead{false}
        , m_hasGParam{false}
        , m_hasParameterList{false}
        , m_hier_ਖੰਡ{false}
        , m_hierParams{false}
        , m_recursive{false}
        , m_recursiveClone{false}
        , m_parameterizedTemplate{false}
        , m_verilatorLib{false} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Module;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਆਭਾਸੀ bool timescaleMatters() ਅਚਲ = 0;
    ਸਤਰਅੰਦਰ bool isDollarUnit() ਅਚਲ;  // Is the $unit package
    ਸਤਰਅੰਦਰ bool isConstPool() ਅਚਲ;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ਅਚਲ string& outer) ਅਚਲ;
    // ACCESSORS
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string origName() ਅਚਲ override { ਵਾਪਸ m_origName; }
    string someInstanceName() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_someInstanceName; }
    ਖਾਲੀ someInstanceName(ਅਚਲ string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ਅਚਲ { ਵਾਪਸ m_defaultDisablep; }
    ਖਾਲੀ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool ਵਿੱਚ_Library() ਅਚਲ { ਵਾਪਸ m_ਵਿੱਚ_Library; }
    ਖਾਲੀ ਵਿੱਚ_Library(bool flag) { m_ਵਿੱਚ_Library = flag; }
    ਖਾਲੀ ਡੂੰਘਾਈ(ਪੂਰਨ ਮੁੱਲ) { m_ਡੂੰਘਾਈ = ਮੁੱਲ; }
    ਪੂਰਨ ਡੂੰਘਾਈ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਡੂੰਘਾਈ; }
    ਖਾਲੀ level(ਪੂਰਨ ਮੁੱਲ) { m_level = ਮੁੱਲ; }
    ਪੂਰਨ level() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_level; }
    string libname() ਅਚਲ { ਵਾਪਸ m_libname; }
    bool is_ਸਿਖਰ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ level() == 1; }
    bool mod_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_mod_ਜਨਤਕ; }
    ਖਾਲੀ mod_ਜਨਤਕ(bool flag) { m_mod_ਜਨਤਕ = flag; }
    bool modTrace() ਅਚਲ { ਵਾਪਸ m_modTrace; }
    ਖਾਲੀ modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ਅਚਲ { ਵਾਪਸ m_ctorVarReset; }
    ਖਾਲੀ ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ਅਚਲ { ਵਾਪਸ m_dead; }
    ਖਾਲੀ dead(bool flag) { m_dead = flag; }
    bool hasGParam() ਅਚਲ { ਵਾਪਸ m_hasGParam; }
    ਖਾਲੀ hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ਅਚਲ { ਵਾਪਸ m_hasParameterList; }
    ਖਾਲੀ hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hier_ਖੰਡ() ਅਚਲ { ਵਾਪਸ m_hier_ਖੰਡ; }
    ਖਾਲੀ hier_ਖੰਡ(bool flag) { m_hier_ਖੰਡ = flag; }
    bool hierParams() ਅਚਲ { ਵਾਪਸ m_hierParams; }
    ਖਾਲੀ hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ਅਚਲ { ਵਾਪਸ m_recursive; }
    ਖਾਲੀ recursive(bool flag) { m_recursive = flag; }
    ਖਾਲੀ recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ਅਚਲ { ਵਾਪਸ m_recursiveClone; }
    bool parameterizedTemplate() ਅਚਲ { ਵਾਪਸ m_parameterizedTemplate; }
    ਖਾਲੀ parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    ਖਾਲੀ verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ਅਚਲ { ਵਾਪਸ m_verilatorLib; }
    VLifetime lifetime() ਅਚਲ { ਵਾਪਸ m_lifetime; }
    ਖਾਲੀ lifetime(ਅਚਲ VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ਅਚਲ { ਵਾਪਸ m_unconnectedDrive; }
    ਖਾਲੀ unconnectedDrive(ਅਚਲ VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_ਗੰਢ_Procedure VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Procedure(VNType t, File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : Ast_ਗੰਢ{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        ਜੋੜੋ_Stmtsp(stmtsp);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Procedure;
    // METHODS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isJustOneBodyStmt() ਅਚਲ { ਵਾਪਸ stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ਅਚਲ { ਵਾਪਸ m_suspendable; }
    ਖਾਲੀ ਸੈੱਟ_Suspendable() { m_suspendable = true; }
    bool needProcess() ਅਚਲ { ਵਾਪਸ m_needProcess; }
    ਖਾਲੀ ਸੈੱਟ_Need_Process() { m_needProcess = true; }
};
class Ast_ਗੰਢ_Range VL_NOT_FINAL : ਜਨਤਕ Ast_ਗੰਢ {
    // A range, sized or unsized
ਸੁਰੱਖਿਅਤ:
    Ast_ਗੰਢ_Range(VNType t, File_ਸਤਰ* fl)
        : Ast_ਗੰਢ{t, fl} {}

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਗੰਢ_Range;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};

// === Concrete node types =====================================================

// === AstNode ===
class Ast_ਸਰਗਰਮ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

ਜਨਤਕ:
    Ast_ਸਰਗਰਮ(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_Sen_ਰੁੱਖ* sentreep)
        : ASTGEN_SUPER_ਸਰਗਰਮ(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_Ast_ਸਰਗਰਮ;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    // Statements are broken into pieces, as some must come before others.
    ਖਾਲੀ sentreep(Ast_Sen_ਰੁੱਖ* nodep) { m_sentreep = nodep; }
    Ast_Sen_ਰੁੱਖ* sentreep() ਅਚਲ { ਵਾਪਸ m_sentreep; }
    // METHODS
    ਸਤਰਅੰਦਰ bool hasClocked() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool hasCombo() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool hasInitial() ਅਚਲ;
    ਸਤਰਅੰਦਰ bool hasStatic() ਅਚਲ;
};
class AstAlias final : ਜਨਤਕ Ast_ਗੰਢ {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
ਜਨਤਕ:
    AstAlias(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : ਜਨਤਕ Ast_ਗੰਢ {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
ਜਨਤਕ:
    AstAliasScope(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
ਜਨਤਕ:
    AstBind(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        ਜੋੜੋ_Cellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Bind Target name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
};
class AstCFunc final : ਜਨਤਕ Ast_ਗੰਢ {
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
    bool m_decl_ਨਿੱਜੀ : 1;  // Declare it private
    bool m_keep_If_ਖਾਲੀ_ਥਾਵਾਂ : 1;  // Keep declaration and definition separate, even if empty
    bool m_slow : 1;  // Slow routine, called once or just at init time
    bool m_func_ਜਨਤਕ : 1;  // From user public task/function
    bool m_isConstructor : 1;  // Is C class constructor
    bool m_isDestructor : 1;  // Is C class destructor
    bool m_is_ਵਿਧੀ : 1;  // Is inside a class definition
    bool m_isLoose : 1;  // Semantically this is a method, but is implemented as a function with
                         // an explicitly passed 'self' pointer as the first argument.  This can
                         // be slightly faster due to __restrict, and we do not declare in header
                         // so adding/removing loose functions doesn't recompile everything.
    bool m_isVirtual : 1;  // Virtual function
    bool m_ਇੰਦਰਾਜ_ਬਿੰਦੂ : 1;  // User may call into this top level function
    bool m_dpi_ਸ਼ੁੱਧ : 1;  // Pure DPI function
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
    ਪੂਰਨ m_ਲਾਗਤ;  // Function call cost
ਜਨਤਕ:
    AstCFunc(File_ਸਤਰ* fl, ਅਚਲ string& name, AstScope* scopep, ਅਚਲ string& rtnType = "")
        : ASTGEN_SUPER_CFunc(fl) {
        m_isConst = VBoolOrUnknown::BU_UNKNOWN;  // Unknown until analyzed
        m_scopep = scopep;
        m_name = name;
        m_rtnType = rtnType;
        m_isStatic = false;
        m_isTrace = false;
        m_dontCombine = false;
        m_decl_ਨਿੱਜੀ = false;
        m_keep_If_ਖਾਲੀ_ਥਾਵਾਂ = false;
        m_slow = false;
        m_func_ਜਨਤਕ = false;
        m_isConstructor = false;
        m_isDestructor = false;
        m_is_ਵਿਧੀ = true;
        m_isLoose = false;
        m_isVirtual = false;
        m_needProcess = false;
        m_ਇੰਦਰਾਜ_ਬਿੰਦੂ = false;
        m_dpi_ਸ਼ੁੱਧ = false;
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
        m_ਲਾਗਤ = v3Global.opt.instr_ਗਿਣਤੀ_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstCFunc* ਅਚਲ asamep = VN_DBG_AS(samep, CFunc);
        ਵਾਪਸ ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ m_ਲਾਗਤ; }
    VBoolOrUnknown isConst() ਅਚਲ { ਵਾਪਸ m_isConst; }
    ਖਾਲੀ isConst(bool flag) { m_isConst.ਸੈੱਟ_True_Or_False(flag); }
    ਖਾਲੀ isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ਅਚਲ { ਵਾਪਸ m_isStatic; }
    ਖਾਲੀ isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_isTrace; }
    ਖਾਲੀ isTrace(bool flag) { m_isTrace = flag; }
    ਖਾਲੀ cname(ਅਚਲ string& name) { m_cname = name; }
    string cname() ਅਚਲ { ਵਾਪਸ m_cname; }
    AstScope* scopep() ਅਚਲ { ਵਾਪਸ m_scopep; }
    ਖਾਲੀ scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ਅਚਲ { ਵਾਪਸ ((m_rtnType == "") ? "void" : m_rtnType); }
    ਖਾਲੀ rtnType(ਅਚਲ string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ਅਚਲ { ਵਾਪਸ m_dontCombine || isTrace() || ਇੰਦਰਾਜ_ਬਿੰਦੂ(); }
    ਖਾਲੀ dontCombine(bool flag) { m_dontCombine = flag; }
    bool decl_ਨਿੱਜੀ() ਅਚਲ { ਵਾਪਸ m_decl_ਨਿੱਜੀ; }
    ਖਾਲੀ decl_ਨਿੱਜੀ(bool flag) { m_decl_ਨਿੱਜੀ = flag; }
    bool keep_If_ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_keep_If_ਖਾਲੀ_ਥਾਵਾਂ; }
    ਖਾਲੀ keep_If_ਖਾਲੀ_ਥਾਵਾਂ(bool flag) { m_keep_If_ਖਾਲੀ_ਥਾਵਾਂ = flag; }
    bool slow() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_slow; }
    ਖਾਲੀ slow(bool flag) { m_slow = flag; }
    bool func_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_func_ਜਨਤਕ; }
    ਖਾਲੀ func_ਜਨਤਕ(bool flag) { m_func_ਜਨਤਕ = flag; }
    ਖਾਲੀ argTypes(ਅਚਲ string& str) { m_argTypes = str; }
    string argTypes() ਅਚਲ { ਵਾਪਸ m_argTypes; }
    ਖਾਲੀ ifdef(ਅਚਲ string& str) { m_ifdef = str; }
    string ifdef() ਅਚਲ { ਵਾਪਸ m_ifdef; }
    bool isConstructor() ਅਚਲ { ਵਾਪਸ m_isConstructor; }
    ਖਾਲੀ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ਅਚਲ { ਵਾਪਸ m_isDestructor; }
    ਖਾਲੀ isDestructor(bool flag) { m_isDestructor = flag; }
    bool is_ਵਿਧੀ() ਅਚਲ { ਵਾਪਸ m_is_ਵਿਧੀ; }
    ਖਾਲੀ is_ਵਿਧੀ(bool flag) { m_is_ਵਿਧੀ = flag; }
    bool isLoose() ਅਚਲ { ਵਾਪਸ m_isLoose; }
    ਖਾਲੀ isLoose(bool flag) { m_isLoose = flag; }
    bool is_Proper_ਵਿਧੀ() ਅਚਲ { ਵਾਪਸ is_ਵਿਧੀ() && !isLoose(); }
    bool isVirtual() ਅਚਲ { ਵਾਪਸ m_isVirtual; }
    ਖਾਲੀ isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ਅਚਲ { ਵਾਪਸ m_needProcess; }
    ਖਾਲੀ ਸੈੱਟ_Need_Process() { m_needProcess = true; }
    bool ਇੰਦਰਾਜ_ਬਿੰਦੂ() ਅਚਲ { ਵਾਪਸ m_ਇੰਦਰਾਜ_ਬਿੰਦੂ; }
    ਖਾਲੀ ਇੰਦਰਾਜ_ਬਿੰਦੂ(bool flag) { m_ਇੰਦਰਾਜ_ਬਿੰਦੂ = flag; }
    bool dpi_ਸ਼ੁੱਧ() ਅਚਲ { ਵਾਪਸ m_dpi_ਸ਼ੁੱਧ; }
    ਖਾਲੀ dpi_ਸ਼ੁੱਧ(bool flag) { m_dpi_ਸ਼ੁੱਧ = flag; }
    bool dpiContext() ਅਚਲ { ਵਾਪਸ m_dpiContext; }
    ਖਾਲੀ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_dpiExportDispatcher; }
    ਖਾਲੀ dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ਅਚਲ { ਵਾਪਸ m_dpiExportImpl; }
    ਖਾਲੀ dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_dpiImportPrototype; }
    ਖਾਲੀ dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ਅਚਲ { ਵਾਪਸ m_dpiImportWrapper; }
    ਖਾਲੀ dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ਅਚਲ { ਵਾਪਸ !m_dpiCDecl.ਖਾਲੀ_ਥਾਵਾਂ(); }
    ਅਚਲ string& dpiCDecl() ਅਚਲ { ਵਾਪਸ m_dpiCDecl; }
    ਖਾਲੀ dpiCDecl(ਅਚਲ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ਅਚਲ { ਵਾਪਸ m_rtnType == "VlCoroutine"; }
    ਖਾਲੀ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ਅਚਲ { ਵਾਪਸ m_recursive; }
    ਖਾਲੀ unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ m_unlikely; }  // Note virtual override
    ਖਾਲੀ noLife(bool flag) { m_noLife = flag; }
    bool noLife() ਅਚਲ { ਵਾਪਸ m_noLife; }
    bool isCovergroupSample() ਅਚਲ { ਵਾਪਸ m_isCovergroupSample; }
    ਖਾਲੀ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ਅਚਲ { ਵਾਪਸ m_voidSelf; }
    ਖਾਲੀ voidSelf(bool flag) { m_voidSelf = flag; }
    ਖਾਲੀ ਲਾਗਤ(ਪੂਰਨ ਲਾਗਤ) { m_ਲਾਗਤ = ਲਾਗਤ; }
    // Special methods
    bool ਖਾਲੀ_ਥਾਵਾਂ_Body() ਅਚਲ {
        ਵਾਪਸ !keep_If_ਖਾਲੀ_ਥਾਵਾਂ() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : ਜਨਤਕ Ast_ਗੰਢ {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ਅਚਲ string m_name;
    ਅਚਲ VUseType m_useType;  // What sort of use this is

ਜਨਤਕ:
    AstCUse(File_ਸਤਰ* fl, VUseType useType, ਅਚਲ string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    VUseType useType() ਅਚਲ { ਵਾਪਸ m_useType; }
};
class AstCell final : ਜਨਤਕ Ast_ਗੰਢ {
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
    File_ਸਤਰ* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    ਪੂਰਨ m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
ਜਨਤਕ:
    AstCell(File_ਸਤਰ* fl, File_ਸਤਰ* mfl, ਅਚਲ string& instName, ਅਚਲ string& modName,
            AstPin* pinsp, AstPin* paramsp, AstRange* rangep)
        : ASTGEN_SUPER_Cell(fl)
        , m_modNameFileline{mfl}
        , m_name{instName}
        , m_origName{instName}
        , m_modName{modName}
        , m_hasIfaceVar{false}
        , m_recursive{false}
        , m_trace{true} {
        ਜੋੜੋ_Pinsp(pinsp);
        ਜੋੜੋ_Paramsp(paramsp);
        ਜੋੜੋ_Rangep(rangep);
    }
    ASTGEN_MEMBERS_AstCell;
    // No cloneRelink, we presume cloneee's want the same module linkages
    ਖਾਲੀ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    // ACCESSORS
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Cell name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string origName() ਅਚਲ override { ਵਾਪਸ m_origName; }  // * = Original name
    ਖਾਲੀ origName(ਅਚਲ string& name) { m_origName = name; }
    string modName() ਅਚਲ { ਵਾਪਸ m_modName; }  // * = Instance name
    ਖਾਲੀ modName(ਅਚਲ string& name) { m_modName = name; }
    File_ਸਤਰ* modNameFileline() ਅਚਲ { ਵਾਪਸ m_modNameFileline; }
    Ast_ਗੰਢ_Module* modp() ਅਚਲ { ਵਾਪਸ m_modp; }  // [AfterLink] = Pointer to module instantiated
    ਖਾਲੀ modp(Ast_ਗੰਢ_Module* nodep) { m_modp = nodep; }
    ਪੂਰਨ arrayIdx() ਅਚਲ { ਵਾਪਸ m_arrayIdx; }
    ਖਾਲੀ arrayIdx(ਪੂਰਨ idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ਅਚਲ { ਵਾਪਸ m_hasIfaceVar; }
    ਖਾਲੀ hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    ਖਾਲੀ trace(bool flag) { m_trace = flag; }
    bool isTrace() ਅਚਲ { ਵਾਪਸ m_trace; }
    ਖਾਲੀ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ਅਚਲ { ਵਾਪਸ m_recursive; }
};
class AstCellInline final : ਜਨਤਕ Ast_ਗੰਢ {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ਅਚਲ std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ਅਚਲ std::string m_origModName;  // __BEGIN__ or __FORK__
ਜਨਤਕ:
    AstCellInline(File_ਸਤਰ* fl, ਅਚਲ std::string& name, ਅਚਲ std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    // ACCESSORS
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    std::string origModName() ਅਚਲ { ਵਾਪਸ m_origModName; }
};
class AstCellInlineScope final : ਜਨਤਕ Ast_ਗੰਢ {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ਅਚਲ std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
ਜਨਤਕ:
    AstCellInlineScope(File_ਸਤਰ* fl, ਅਚਲ std::string& name, ਅਚਲ std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    // ACCESSORS
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ std::string& name) override { m_name = name; }
    std::string origModName() ਅਚਲ { ਵਾਪਸ m_origModName; }
};
class Ast_Cg_Option_ਸੌਂਪੋ final : ਜਨਤਕ Ast_ਗੰਢ {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ਅਚਲ VCoverOptionType m_optType;  // Option type
    ਅਚਲ string m_name;  // Original option name (for diagnostics on unknown options)
    ਅਚਲ bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
ਜਨਤਕ:
    Ast_Cg_Option_ਸੌਂਪੋ(File_ਸਤਰ* fl, bool typeOption, VCoverOptionType optType,
                      ਅਚਲ string& rawName, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cg_Option_ਸੌਂਪੋ(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_Ast_Cg_Option_ਸੌਂਪੋ;
    // ACCESSORS
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    VCoverOptionType optType() ਅਚਲ { ਵਾਪਸ m_optType; }
    bool typeOption() ਅਚਲ { ਵਾਪਸ m_typeOption; }
};
class AstClassExtends final : ਜਨਤਕ Ast_ਗੰਢ {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ਅਚਲ bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

ਜਨਤਕ:
    AstClassExtends(File_ਸਤਰ* fl, Ast_ਗੰਢ* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ਅਚਲ;
    AstClass* classp() ਅਚਲ;  // Like above, but throws error if nulll
    bool isImplements() ਅਚਲ { ਵਾਪਸ m_isImplements; }
    ਖਾਲੀ parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ਅਚਲ { ਵਾਪਸ m_parameterized; }
};
class AstClocking final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ਅਚਲ bool m_isGlobal;  // True if global clocking

ਜਨਤਕ:
    AstClocking(File_ਸਤਰ* fl, ਅਚਲ std::string& name, Ast_Sen_ਰੁੱਖ* sentreep, Ast_ਗੰਢ* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool isDefault() ਅਚਲ { ਵਾਪਸ m_isDefault; }
    bool isGlobal() ਅਚਲ { ਵਾਪਸ m_isGlobal; }
    AstVar* ensureEventp(bool ਬੱਚਾ_DType = false);
    ਖਾਲੀ makeDefault() { m_isDefault = true; }
};
class Ast_Clocking_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

ਜਨਤਕ:
    Ast_Clocking_ਵਸਤੂ(File_ਸਤਰ* fl, VDirection direction, Ast_ਗੰਢ_ਵਿਅੰਜਕ* skewp, Ast_ਗੰਢ* clockingDeclp)
        : ASTGEN_SUPER_Clocking_ਵਸਤੂ(fl) {
        m_direction = direction;
        this->skewp(skewp);
        ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ clkAssignp = VN_CAST(clockingDeclp, ਸੌਂਪੋ)) {
            this->assignp(clkAssignp);
        } ਨਹੀਂਤਾਂ {
            exprp(VN_AS(clockingDeclp, ਗੰਢ_ਵਿਅੰਜਕ));
        }
    }
    ASTGEN_MEMBERS_Ast_Clocking_ਵਸਤੂ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VDirection direction() ਅਚਲ { ਵਾਪਸ m_direction; }
    Ast_Clocking_ਵਸਤੂ* outputp() ਅਚਲ { ਵਾਪਸ m_outputp; }
    ਖਾਲੀ outputp(Ast_Clocking_ਵਸਤੂ* outputp) { m_outputp = outputp; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
};
class AstConfig final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

ਜਨਤਕ:
    AstConfig(File_ਸਤਰ* fl, ਅਚਲ std::string& libname, ਅਚਲ std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_libname + "." + m_configname; }
    std::string libname() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_libname; }
    std::string configname() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_configname; }
};
class AstConfigCell final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

ਜਨਤਕ:
    AstConfigCell(File_ਸਤਰ* fl, ਅਚਲ std::string& libname, ਅਚਲ std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_libname + "." + m_cellname; }
    std::string libname() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_libname; }
    std::string cellname() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_cellname; }
};
class Ast_Config_ਨਿਯਮ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ਅਚਲ bool m_isCell;  // Declared as "cell" versus "instance"

ਜਨਤਕ:
    Ast_Config_ਨਿਯਮ(File_ਸਤਰ* fl, Ast_ਗੰਢ* cellp, Ast_ਗੰਢ* usep, bool isCell)
        : ASTGEN_SUPER_Config_ਨਿਯਮ(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        ਜੋੜੋ_Usep(usep);
    }
    ASTGEN_MEMBERS_Ast_Config_ਨਿਯਮ;
    bool isCell() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_isCell; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class AstConfigUse final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ਅਚਲ bool m_isConfig;  // ":config"; Config, not module/primitive name

ਜਨਤਕ:
    AstConfigUse(File_ਸਤਰ* fl, ਅਚਲ std::string& libname, ਅਚਲ std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        ਜੋੜੋ_Paramsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_libname + "." + m_cellname; }
    std::string libname() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_libname; }
    std::string cellname() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_cellname; }
    bool isConfig() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_isConfig; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class AstConstraint final : ਜਨਤਕ Ast_ਗੰਢ {
    // Constraint
    // @astgen op1 := itemsp : List[AstNode]
    // @astgen op2 := classOrPackagep : Optional[AstNode]
    string m_name;  // Name of constraint
    VBaseOverride m_ਅਧਾਰ_Override;  // BaseOverride (inital/final/extends)
    bool m_isExternDef = false;  // Extern prototype definition
    bool m_isExternExplicit = false;  // Explicit prototype declaration (has extern)
    bool m_isExternProto = false;  // Prototype declaration (implicit or explicit)
    bool m_is_Kwd_ਸ਼ੁੱਧ = false;  // Pure constraint
    bool m_isStatic = false;  // Static constraint
ਜਨਤਕ:
    AstConstraint(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Scope name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    ਖਾਲੀ ਅਧਾਰ_Override(ਅਚਲ VBaseOverride& flag) { m_ਅਧਾਰ_Override = flag; }
    VBaseOverride ਅਧਾਰ_Override() ਅਚਲ { ਵਾਪਸ m_ਅਧਾਰ_Override; }
    bool isExternDef() ਅਚਲ { ਵਾਪਸ m_isExternDef; }
    ਖਾਲੀ isExternDef(bool flag) { m_isExternDef = flag; }
    ਖਾਲੀ isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ਅਚਲ { ਵਾਪਸ m_isExternExplicit; }
    ਖਾਲੀ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ਅਚਲ { ਵਾਪਸ m_isExternProto; }
    ਖਾਲੀ is_Kwd_ਸ਼ੁੱਧ(bool flag) { m_is_Kwd_ਸ਼ੁੱਧ = flag; }
    bool is_Kwd_ਸ਼ੁੱਧ() ਅਚਲ { ਵਾਪਸ m_is_Kwd_ਸ਼ੁੱਧ; }
    ਖਾਲੀ isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ਅਚਲ { ਵਾਪਸ m_isStatic; }
};
class AstCoverBin final : ਜਨਤਕ Ast_ਗੰਢ {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    ਅਚਲ string m_name;  // Base name of the bin
    ਅਚਲ VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

ਜਨਤਕ:
    AstCoverBin(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_ਗੰਢ_ਵਿਅੰਜਕ* iffp = nullptr)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{isIllegal ? VCoverBinsType::BINS_ILLEGAL
                               : (isIgnore ? VCoverBinsType::BINS_IGNORE
                                           : (isWildcard ? VCoverBinsType::BINS_WILDCARD
                                                         : VCoverBinsType::BINS_USER))}
        , m_isWildcard{isWildcard} {
        ਜੋੜੋ_Rangesp(rangesp);
        this->iffp(iffp);
    }
    // Constructor for automatic bins
    AstCoverBin(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_ਸਤਰ* fl, ਅਚਲ string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_Cover_Trans_ਸੈੱਟ* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        ਜੋੜੋ_Transp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    VCoverBinsType binsType() ਅਚਲ { ਵਾਪਸ m_binsType; }
    bool isWildcard() ਅਚਲ { ਵਾਪਸ m_isWildcard; }
    bool isArray() ਅਚਲ { ਵਾਪਸ m_isArray; }
    ਖਾਲੀ isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : ਜਨਤਕ Ast_ਗੰਢ {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ਅਚਲ bool m_isNegated;  // Complement the selection within the cross product

ਜਨਤਕ:
    AstCoverBinsof(File_ਸਤਰ* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_ਗੰਢ* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        ਜੋੜੋ_Rangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    bool isNegated() ਅਚਲ { ਵਾਪਸ m_isNegated; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_START
        ਅਚਲ AstCoverBinsof* ਅਚਲ asamep = VN_DBG_AS(samep, CoverBinsof);
        ਵਾਪਸ m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : ਜਨਤਕ Ast_ਗੰਢ {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ਅਚਲ string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ਅਚਲ VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

ਜਨਤਕ:
    AstCoverCrossBin(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* selectp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    string verilogKwd() ਅਚਲ override;
    VCoverBinsType binsType() ਅਚਲ { ਵਾਪਸ m_binsType; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_START
        ਅਚਲ AstCoverCrossBin* ਅਚਲ asamep = VN_DBG_AS(samep, CoverCrossBin);
        ਵਾਪਸ m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : ਜਨਤਕ Ast_ਗੰਢ {
    // The complete bin-tuple space of the enclosing cross
    ਅਚਲ string m_name;  // Referenced cross name

ਜਨਤਕ:
    AstCoverCrossRef(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_START
        ਵਾਪਸ m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : ਜਨਤਕ Ast_ਗੰਢ {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ਅਚਲ bool m_isOr;  // Union (||), rather than intersection (&&)

ਜਨਤਕ:
    AstCoverCrossSelect(File_ਸਤਰ* fl, Ast_ਗੰਢ* lhsp, Ast_ਗੰਢ* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool isOr() ਅਚਲ { ਵਾਪਸ m_isOr; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ isOr() ? "||" : "&&"; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_START
        ਵਾਪਸ m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : ਜਨਤਕ Ast_ਗੰਢ {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
ਜਨਤਕ:
    AstCoverCrossWith(File_ਸਤਰ* fl, Ast_ਗੰਢ* selectp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filterp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : ਜਨਤਕ Ast_ਗੰਢ {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ਅਚਲ VCoverOptionType m_optType;  // Option being assigned
    ਅਚਲ bool m_typeOption;  // type_option (vs option)

ਜਨਤਕ:
    AstCoverOption(File_ਸਤਰ* fl, bool typeOption, VCoverOptionType optType, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VCoverOptionType optType() ਅਚਲ { ਵਾਪਸ m_optType; }
    bool typeOption() ਅਚਲ { ਵਾਪਸ m_typeOption; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {  // LCOV_EXCL_START
        ਅਚਲ AstCoverOption* ਅਚਲ asamep = VN_DBG_AS(samep, CoverOption);
        ਵਾਪਸ m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : ਜਨਤਕ Ast_ਗੰਢ {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
ਜਨਤਕ:
    AstCoverToggle(File_ਸਤਰ* fl, AstCoverInc* incp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* origp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ 3 + INSTR_ਗਿਣਤੀ_BRANCH + INSTR_ਗਿਣਤੀ_LD; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ true; }
    bool isOutputter() override {
        ਵਾਪਸ false;  // Though the AstCoverInc under this is an outputter
    }
};
class Ast_Cover_Trans_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ਅਚਲ VTransRepType m_repType;

ਜਨਤਕ:
    Ast_Cover_Trans_ਵਸਤੂ(File_ਸਤਰ* fl, Ast_ਗੰਢ* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_Cover_Trans_ਵਸਤੂ(fl)
        , m_repType{repType} {
        ਜੋੜੋ_Valuesp(valuesp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_ਵਸਤੂ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class Ast_Cover_Trans_ਸੈੱਟ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
ਜਨਤਕ:
    Ast_Cover_Trans_ਸੈੱਟ(File_ਸਤਰ* fl, Ast_Cover_Trans_ਵਸਤੂ* itemsp)
        : ASTGEN_SUPER_Cover_Trans_ਸੈੱਟ(fl) {
        ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Cover_Trans_ਸੈੱਟ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class AstCoverWith final : ਜਨਤਕ Ast_ਗੰਢ {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
ਜਨਤਕ:
    AstCoverWith(File_ਸਤਰ* fl, Ast_ਗੰਢ* subp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        ਜੋੜੋ_Subp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : ਜਨਤਕ Ast_ਗੰਢ {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

ਜਨਤਕ:
    AstCovergroup(File_ਸਤਰ* fl, ਅਚਲ string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_ਗੰਢ* membersp, Ast_Sen_ਰੁੱਖ* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        ਜੋੜੋ_Argsp(argsp);
        ਜੋੜੋ_Sample_Argsp(sampleArgsp);
        ਜੋੜੋ_Membersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override { ਵਾਪਸ true; }
};
class AstCoverpointRef final : ਜਨਤਕ Ast_ਗੰਢ {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ਅਚਲ string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

ਜਨਤਕ:
    AstCoverpointRef(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
};
class AstDefParam final : ਜਨਤਕ Ast_ਗੰਢ {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
ਜਨਤਕ:
    AstDefParam(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* pathp, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstDefaultClocking final : ਜਨਤਕ Ast_ਗੰਢ {
    std::string m_name;  // Clocking block name

ਜਨਤਕ:
    AstDefaultClocking(File_ਸਤਰ* fl, ਅਚਲ std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
};
class AstDefaultDisable final : ਜਨਤਕ Ast_ਗੰਢ {
    // @astgen op1 := condp : AstNodeExpr

ਜਨਤਕ:
    AstDefaultDisable(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : ਜਨਤਕ Ast_ਗੰਢ {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
ਜਨਤਕ:
    AstDpiExport(File_ਸਤਰ* fl, ਅਚਲ string& vname, ਅਚਲ string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string cname() ਅਚਲ { ਵਾਪਸ m_cname; }
    ਖਾਲੀ cname(ਅਚਲ string& cname) { m_cname = cname; }
};
class AstElabDisplay final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

ਜਨਤਕ:
    ਸਤਰਅੰਦਰ AstElabDisplay(File_ਸਤਰ* fl, VDisplayType dispType, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!fmtp());
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ਵਾਪਸ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ਅਚਲ override { ਵਾਪਸ true; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    ਪੂਰਨ instr_ਗਿਣਤੀ() ਅਚਲ override { ਵਾਪਸ INSTR_ਗਿਣਤੀ_PLI; }
    VDisplayType displayType() ਅਚਲ { ਵਾਪਸ m_displayType; }
    ਖਾਲੀ displayType(VDisplayType type) { m_displayType = type; }
};
class Ast_ਖਾਲੀ_ਥਾਵਾਂ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Represents something missing, e.g. a missing argument in FOREACH
ਜਨਤਕ:
    explicit Ast_ਖਾਲੀ_ਥਾਵਾਂ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_ਖਾਲੀ_ਥਾਵਾਂ(fl) {}
    ASTGEN_MEMBERS_Ast_ਖਾਲੀ_ਥਾਵਾਂ;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class Ast_Gen_Case_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
ਜਨਤਕ:
    Ast_Gen_Case_ਵਸਤੂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condsp, Ast_ਗੰਢ* itemsp)
        : ASTGEN_SUPER_Gen_Case_ਵਸਤੂ(fl) {
        ਜੋੜੋ_Condsp(condsp);
        ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_Case_ਵਸਤੂ;
    bool isDefault() ਅਚਲ { ਵਾਪਸ condsp() == nullptr; }
};
class AstImplicit final : ਜਨਤਕ Ast_ਗੰਢ {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
ਜਨਤਕ:
    AstImplicit(File_ਸਤਰ* fl, Ast_ਗੰਢ* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        ਜੋੜੋ_Exprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : ਜਨਤਕ Ast_ਗੰਢ {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_ਮਾਪਾ_Name;  // Hierarchical path of the declaring module, before inlining
    string m_ਅਧਾਰ_Name;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
ਜਨਤਕ:
    AstIntfRef(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& ਮਾਪਾ_Name, ਅਚਲ string& ਅਧਾਰ_Name,
               ਅਚਲ string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_ਮਾਪਾ_Name{ਮਾਪਾ_Name}
        , m_ਅਧਾਰ_Name{ਅਧਾਰ_Name}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    string ਮਾਪਾ_Name() ਅਚਲ { ਵਾਪਸ m_ਮਾਪਾ_Name; }
    string ਅਧਾਰ_Name() ਅਚਲ { ਵਾਪਸ m_ਅਧਾਰ_Name; }
    string modportName() ਅਚਲ { ਵਾਪਸ m_modportName; }
};
class AstLibrary final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

ਜਨਤਕ:
    AstLibrary(File_ਸਤਰ* fl, ਅਚਲ std::string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* filesp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        ਜੋੜੋ_Filesp(filesp);
        ਜੋੜੋ_Incdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
};
class AstModport final : ਜਨਤਕ Ast_ਗੰਢ {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
ਜਨਤਕ:
    AstModport(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        ਜੋੜੋ_Varsp(varsp);
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "modport"; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : ਜਨਤਕ Ast_ਗੰਢ {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
ਜਨਤਕ:
    AstModportClockingRef(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    AstClocking* clockingp() ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ m_clockingp;
    }  // [After Link] Pointer to clocking block
    ਖਾਲੀ clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : ਜਨਤਕ Ast_ਗੰਢ {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
ਜਨਤਕ:
    AstModportFTaskRef(File_ਸਤਰ* fl, ਅਚਲ string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool isImport() ਅਚਲ { ਵਾਪਸ !m_export; }
    bool isExport() ਅਚਲ { ਵਾਪਸ m_export; }
    Ast_ਗੰਢ_FTask* ftaskp() ਅਚਲ { ਵਾਪਸ m_ftaskp; }  // [After Link] Pointer to variable
    ਖਾਲੀ ftaskp(Ast_ਗੰਢ_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : ਜਨਤਕ Ast_ਗੰਢ {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ਅਚਲ VDirection m_direction;  // Direction of the variable (in/out)
ਜਨਤਕ:
    AstModportVarRef(File_ਸਤਰ* fl, ਅਚਲ string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    VDirection direction() ਅਚਲ { ਵਾਪਸ m_direction; }
    AstVar* varp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_varp; }  // [After Link] Pointer to variable
    ਖਾਲੀ varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : ਜਨਤਕ Ast_ਗੰਢ {
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
    std::string m_resolved_ਸਿਖਰ_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::ਸੈੱਟ<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ਅਚਲ AstConst*, string> m_const_Orig_Param_ਨਾਂ;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ*, ਅਚਲ Ast_ਗੰਢ_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::ENUM_ਸਿਰਾ> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::ENUM_ਸਿਰਾ> m_dumpTriggersFuncps{};

ਜਨਤਕ:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override;
    ਖਾਲੀ ਧੱਕੋ_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.ਪਾਓ(varp); }
    ਅਚਲ std::ਸੈੱਟ<AstVar*>& deferredParamVarps() ਅਚਲ { ਵਾਪਸ m_deferredParamVarps; }
    ਖਾਲੀ clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    ਖਾਲੀ deleteContents();
    ਖਾਲੀ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ "$root"; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    Ast_ਗੰਢ_Module* ਸਿਖਰ_Modulep() ਅਚਲ VL_MT_STABLE {  // Top module in hierarchy
        ਵਾਪਸ modulesp();  // First one in the list, for now
    }
    Ast_Type_ਸਾਰਣੀ* typeTablep() { ਵਾਪਸ m_typeTablep; }
    string astConstOrigParamName(ਅਚਲ AstConst* nodep) ਅਚਲ;
    ਖਾਲੀ astConstOrigParamName(ਅਚਲ AstConst* nodep, ਅਚਲ string& name);
    ਖਾਲੀ astConstOrigParamNameErase(ਅਚਲ AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ਅਚਲ Ast_ਗੰਢ_Module* containingModule(ਅਚਲ Ast_ਗੰਢ* nodep);
    // Forget remembered modules, as the tree has moved.
    ਖਾਲੀ clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ਅਚਲ { ਵਾਪਸ m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ਅਚਲ { ਵਾਪਸ m_dollarUnitPkgp; }
    ਖਾਲੀ dollarUnitPkgp(AstPackage* ਅਚਲ packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ਅਚਲ { ਵਾਪਸ m_dpiExportTriggerp; }
    ਖਾਲੀ dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ਅਚਲ { ਵਾਪਸ m_delaySchedulerp; }
    ਖਾਲੀ delaySchedulerp(AstVar* ਅਚਲ varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ਅਚਲ { ਵਾਪਸ m_nbaEventp; }
    ਖਾਲੀ nbaEventp(AstVarScope* ਅਚਲ varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ਅਚਲ { ਵਾਪਸ m_nbaEventTriggerp; }
    ਖਾਲੀ nbaEventTriggerp(AstVarScope* ਅਚਲ varScopep) { m_nbaEventTriggerp = varScopep; }
    ਖਾਲੀ stdPackagep(AstPackage* ਅਚਲ packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ਅਚਲ { ਵਾਪਸ m_stdPackagep; }
    ਖਾਲੀ stdPackageProcessp(AstClass* ਅਚਲ classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ਅਚਲ { ਵਾਪਸ m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_ਸਤਰ*) ਅਚਲ;
    Ast_ਸਿਖਰ_Scope* ਸਿਖਰ_Scopep() ਅਚਲ { ਵਾਪਸ m_ਸਿਖਰ_Scopep; }
    ਖਾਲੀ create_ਸਿਖਰ_Scope(AstScope* scopep);
    VTimescale timeunit() ਅਚਲ { ਵਾਪਸ m_timeunit; }
    ਖਾਲੀ timeunit(ਅਚਲ VTimescale& ਮੁੱਲ) { m_timeunit = ਮੁੱਲ; }
    VTimescale timeprecision() ਅਚਲ { ਵਾਪਸ m_timeprecision; }
    ਖਾਲੀ ਸਮਾਂ_Init() {
        m_timeunit = v3Global.opt.ਸਮਾਂ_Default_Unit();
        m_timeprecision = v3Global.opt.ਸਮਾਂ_Default_Prec();
    }
    ਖਾਲੀ timeprecision_ਮਿਲਾਓ(File_ਸਤਰ*, ਅਚਲ VTimescale& ਮੁੱਲ);
    ਖਾਲੀ timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ਅਚਲ { ਵਾਪਸ m_timescaleSpecified; }
    ਅਚਲ std::string& resolved_ਸਿਖਰ_Module_Name() ਅਚਲ { ਵਾਪਸ m_resolved_ਸਿਖਰ_Module_Name; }
    ਖਾਲੀ resolved_ਸਿਖਰ_Module_Name(ਅਚਲ std::string& ਮੁੱਲ) { m_resolved_ਸਿਖਰ_Module_Name = ਮੁੱਲ; }
    uint32_t nTraceCodes() ਅਚਲ { ਵਾਪਸ m_nTraceCodes; }
    ਖਾਲੀ nTraceCodes(uint32_t ਮੁੱਲ) { m_nTraceCodes = ਮੁੱਲ; }
    AstVarScope* stl_ਪਹਿਲਾ_Iterationp();
    ਖਾਲੀ clear_Stl_ਪਹਿਲਾ_Iterationp() { m_stl_ਪਹਿਲਾ_Iterationp = nullptr; }
    ਅਚਲ std::string trace_Lib_ਸਿਖਰ_Name() ਅਚਲ {
        ਅਚਲ std::string& name = resolved_ਸਿਖਰ_Module_Name();
        ਵਾਪਸ prettyName(name.ਖਾਲੀ_ਥਾਵਾਂ() ? v3Global.rootp()->ਸਿਖਰ_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    ਖਾਲੀ ਜੋੜੋ_Eval_Stats(ਅਚਲ std::string& phase);
};
class AstPackageExport final : ਜਨਤਕ Ast_ਗੰਢ {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ਜਨਤਕ:
    AstPackageExport(File_ਸਤਰ* fl, ਅਚਲ string& pkgName, ਅਚਲ string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    string pkgName() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_pkgName; }
    string prettyPkgNameQ() ਅਚਲ { ਵਾਪਸ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ਅਚਲ { ਵਾਪਸ m_packagep; }
    ਖਾਲੀ packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class Ast_Package_Export_ਤਾਰਾ_ਤਾਰਾ final : ਜਨਤਕ Ast_ਗੰਢ {
    // A package export *::* declaration
ਜਨਤਕ:
    // cppcheck-suppress noExplicitConstructor
    Ast_Package_Export_ਤਾਰਾ_ਤਾਰਾ(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_Package_Export_ਤਾਰਾ_ਤਾਰਾ(fl) {}
    ASTGEN_MEMBERS_Ast_Package_Export_ਤਾਰਾ_ਤਾਰਾ;
};
class AstPackageImport final : ਜਨਤਕ Ast_ਗੰਢ {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ਜਨਤਕ:
    AstPackageImport(File_ਸਤਰ* fl, AstPackage* packagep, ਅਚਲ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_ਸਤਰ* fl, ਅਚਲ string& pkgName, ਅਚਲ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    string pkgName() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_pkgName; }
    string prettyPkgNameQ() ਅਚਲ { ਵਾਪਸ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ਅਚਲ { ਵਾਪਸ m_packagep; }
    ਖਾਲੀ packagep(AstPackage* nodep) { m_packagep = nodep; }

ਨਿਜੀ:
    ਖਾਲੀ pkgNameFrom();
};
class AstPin final : ਜਨਤਕ Ast_ਗੰਢ {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    ਪੂਰਨ m_pin_ਅੰਕ;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_param_ਰਸਤਾ;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
ਜਨਤਕ:
    AstPin(File_ਸਤਰ* fl, ਪੂਰਨ pin_ਅੰਕ, ਅਚਲ string& name, Ast_ਗੰਢ* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_ਅੰਕ{pin_ਅੰਕ}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    ਖਾਲੀ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Pin name, ""=go by number
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string prettyOperatorName() ਅਚਲ override;
    bool dot_ਤਾਰਾ() ਅਚਲ { ਵਾਪਸ name() == ".*"; }  // Fake name for .* connections until linked
    ਪੂਰਨ pin_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_pin_ਅੰਕ; }
    AstVar* modVarp() ਅਚਲ { ਵਾਪਸ m_modVarp; }  // [After Link] Pointer to variable
    ਖਾਲੀ modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ਅਚਲ { ਵਾਪਸ m_modPTypep; }
    ਖਾਲੀ modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ਅਚਲ { ਵਾਪਸ m_param; }
    ਖਾਲੀ param(bool flag) { m_param = flag; }
    ਅਚਲ string& param_ਰਸਤਾ() ਅਚਲ { ਵਾਪਸ m_param_ਰਸਤਾ; }
    ਖਾਲੀ param_ਰਸਤਾ(ਅਚਲ string& ਰਸਤਾ) { m_param_ਰਸਤਾ = ਰਸਤਾ; }
    bool svDotName() ਅਚਲ { ਵਾਪਸ m_svDotName; }
    ਖਾਲੀ svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ਅਚਲ { ਵਾਪਸ m_svImplicit; }
    ਖਾਲੀ svImplicit(bool flag) { m_svImplicit = flag; }
};
class Ast_ਪੋਰਟ final : ਜਨਤਕ Ast_ਗੰਢ {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ਅਚਲ ਪੂਰਨ m_pin_ਅੰਕ;  // Pin number
    ਅਚਲ string m_name;  // Name of pin
ਜਨਤਕ:
    Ast_ਪੋਰਟ(File_ਸਤਰ* fl, ਪੂਰਨ pinnum, ਅਚਲ string& name)
        : ASTGEN_SUPER_ਪੋਰਟ(fl)
        , m_pin_ਅੰਕ{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ਪੋਰਟ;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Port name
    ਪੂਰਨ pin_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_pin_ਅੰਕ; }  // * = Pin number, for order based instantiation
};
class AstPragma final : ਜਨਤਕ Ast_ਗੰਢ {
    ਅਚਲ VPragmaType m_pragType;  // Type of pragma
    ਅਚਲ VTimescale m_timescale;  // For TIMEUNIT_SET
ਜਨਤਕ:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_ਸਤਰ* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_ਸੈੱਟ, fl, "Should use other constructor");
    }
    AstPragma(File_ਸਤਰ* fl, ਅਚਲ VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_ਸੈੱਟ}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VPragmaType pragType() ਅਚਲ { ਵਾਪਸ m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ਅਚਲ override { ਵਾਪਸ false; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ਅਚਲ { ਵਾਪਸ m_timescale; }
};
class AstPropSpec final : ਜਨਤਕ Ast_ਗੰਢ {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

ਜਨਤਕ:
    AstPropSpec(File_ਸਤਰ* fl, Ast_Sen_ਵਸਤੂ* sensesp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* disablep, Ast_ਗੰਢ* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ {
        ਵਾਪਸ true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ਅਚਲ { ਵਾਪਸ m_propStrength; }
};
class AstPull final : ਜਨਤਕ Ast_ਗੰਢ {
    // @astgen op1 := lhsp : AstNodeExpr

    ਅਚਲ bool m_direction;

ਜਨਤਕ:
    AstPull(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ਅਚਲ { ਵਾਪਸ m_direction; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class AstScope final : ਜਨਤਕ Ast_ਗੰਢ {
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
ਜਨਤਕ:
    AstScope(File_ਸਤਰ* fl, Ast_ਗੰਢ_Module* modp, ਅਚਲ string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!m_modp);
        ਵਾਪਸ nullptr;
    }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Scope name
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    string pretty_Name_ਸੁਨੇਹਾ() ਅਚਲ override;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    string nameDotless() ਅਚਲ;
    Ast_ਗੰਢ_Module* modp() ਅਚਲ { ਵਾਪਸ m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    Ast_ਸਰਗਰਮ* comboActivep(bool create);
    //
    AstScope* aboveScopep() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_aboveScopep; }
    ਖਾਲੀ aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ਅਚਲ { ਵਾਪਸ m_aboveCellp; }
    ਖਾਲੀ aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_ਸਿਖਰ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ aboveScopep() == nullptr; }  // At top of hierarchy
};
class Ast_Sen_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_ਕਿਨਾਰਾ_Type;  // Edge type
ਜਨਤਕ:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    Ast_Sen_ਵਸਤੂ(File_ਸਤਰ* fl, VEdgeType ਕਿਨਾਰਾ_Type, Ast_ਗੰਢ_ਵਿਅੰਜਕ* senp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp = nullptr)
        : ASTGEN_SUPER_Sen_ਵਸਤੂ(fl)
        , m_ਕਿਨਾਰਾ_Type{ਕਿਨਾਰਾ_Type} {
        this->sensp(senp);
        this->condp(condp);
    }
    Ast_Sen_ਵਸਤੂ(File_ਸਤਰ* fl, Combo)
        : ASTGEN_SUPER_Sen_ਵਸਤੂ(fl)
        , m_ਕਿਨਾਰਾ_Type{VEdgeType::ET_COMBO} {}
    Ast_Sen_ਵਸਤੂ(File_ਸਤਰ* fl, Static)
        : ASTGEN_SUPER_Sen_ਵਸਤੂ(fl)
        , m_ਕਿਨਾਰਾ_Type{VEdgeType::ET_STATIC} {}
    Ast_Sen_ਵਸਤੂ(File_ਸਤਰ* fl, Initial)
        : ASTGEN_SUPER_Sen_ਵਸਤੂ(fl)
        , m_ਕਿਨਾਰਾ_Type{VEdgeType::ET_INITIAL} {}
    Ast_Sen_ਵਸਤੂ(File_ਸਤਰ* fl, InitialNBA)
        : ASTGEN_SUPER_Sen_ਵਸਤੂ(fl)
        , m_ਕਿਨਾਰਾ_Type{VEdgeType::ET_INITIAL_NBA} {}
    Ast_Sen_ਵਸਤੂ(File_ਸਤਰ* fl, Final)
        : ASTGEN_SUPER_Sen_ਵਸਤੂ(fl)
        , m_ਕਿਨਾਰਾ_Type{VEdgeType::ET_FINAL} {}
    Ast_Sen_ਵਸਤੂ(File_ਸਤਰ* fl, Never)
        : ASTGEN_SUPER_Sen_ਵਸਤੂ(fl)
        , m_ਕਿਨਾਰਾ_Type{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_Ast_Sen_ਵਸਤੂ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VN_DBG_AS(samep, Sen_ਵਸਤੂ)->ਕਿਨਾਰਾ_Type();
    }
    VEdgeType ਕਿਨਾਰਾ_Type() ਅਚਲ { ਵਾਪਸ m_ਕਿਨਾਰਾ_Type; }
    ਖਾਲੀ ਕਿਨਾਰਾ_Type(VEdgeType type) {
        m_ਕਿਨਾਰਾ_Type = type;
        edit_ਗਿਣਤੀ_Inc();
    }
    Ast_ਗੰਢ_Var_Ref* varrefp() ਅਚਲ { ਵਾਪਸ VN_CAST(sensp(), ਗੰਢ_Var_Ref); }
    //
    bool isClocked() ਅਚਲ { ਵਾਪਸ ਕਿਨਾਰਾ_Type().clockedStmt(); }
    bool is_Combo_Or_ਤਾਰਾ() ਅਚਲ {
        ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VEdgeType::ET_COMBO || ਕਿਨਾਰਾ_Type() == VEdgeType::ET_COMBO_ਤਾਰਾ;
    }
    bool is_Combo_ਤਾਰਾ() ਅਚਲ { ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VEdgeType::ET_COMBO_ਤਾਰਾ; }
    bool isHybrid() ਅਚਲ { ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VEdgeType::ET_HYBRID; }
    bool isStatic() ਅਚਲ { ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VEdgeType::ET_STATIC; }
    bool isInitial() ਅਚਲ { ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VEdgeType::ET_INITIAL; }
    bool isFinal() ਅਚਲ { ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VEdgeType::ET_FINAL; }
    bool isNever() ਅਚਲ { ਵਾਪਸ ਕਿਨਾਰਾ_Type() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_ਰੁੱਖ final : ਜਨਤਕ Ast_ਗੰਢ {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
ਜਨਤਕ:
    Ast_Sen_ਰੁੱਖ(File_ਸਤਰ* fl, Ast_Sen_ਵਸਤੂ* sensesp)
        : ASTGEN_SUPER_Sen_ਰੁੱਖ(fl) {
        ਜੋੜੋ_Sensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_ਰੁੱਖ;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    // METHODS
    bool hasClocked() ਅਚਲ;  // Includes a clocked statement
    bool has_ਕਿਨਾਰਾ() ਅਚਲ;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ਅਚਲ;  // Includes a STATIC SenItem
    bool hasInitial() ਅਚਲ;  // Includes a INITIAL SenItem
    bool hasFinal() ਅਚਲ;  // Includes a FINAL SenItem
    bool hasCombo() ਅਚਲ;  // Includes a COMBO SenItem
    bool hasHybrid() ਅਚਲ;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : ਜਨਤਕ Ast_ਗੰਢ {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

ਜਨਤਕ:
    AstStrengthSpec(File_ਸਤਰ* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { ਵਾਪਸ m_s0; }
    VStrength strength1() { ਵਾਪਸ m_s1; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class AstSystemCSection final : ਜਨਤਕ Ast_ਗੰਢ {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ਅਚਲ VSystemCSectionType m_sectionType;  // The section type
    std::string m_ਪਾਠ;  // The text content

ਜਨਤਕ:
    AstSystemCSection(File_ਸਤਰ* fl, VSystemCSectionType sectionType, ਅਚਲ std::string& ਪਾਠ)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_ਪਾਠ{ਪਾਠ} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ਅਚਲ { ਵਾਪਸ m_sectionType; }
    ਅਚਲ std::string& ਪਾਠ() ਅਚਲ { ਵਾਪਸ m_ਪਾਠ; }
    ਖਾਲੀ ਪਾਠ(ਅਚਲ std::string& ਮੁੱਲ) { m_ਪਾਠ = ਮੁੱਲ; }
    ਖਾਲੀ dump(std::ostream&) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream&) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ*) ਅਚਲ override { ਵਾਪਸ false; }
    bool is_ਸ਼ੁੱਧ() override { ਵਾਪਸ false; }
    bool isOutputter() override { ਵਾਪਸ true; }
};
class Ast_ਪਾਠ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ਅਚਲ std::string m_ਪਾਠ;  // The text to emit
ਜਨਤਕ:
    Ast_ਪਾਠ(File_ਸਤਰ* fl, ਅਚਲ std::string& ਪਾਠ)
        : ASTGEN_SUPER_ਪਾਠ(fl)
        , m_ਪਾਠ{ਪਾਠ} {}
    ASTGEN_MEMBERS_Ast_ਪਾਠ;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ ਪਾਠ() == VN_DBG_AS(samep, ਪਾਠ_2)->ਪਾਠ();
    }
    ਅਚਲ std::string& ਪਾਠ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਪਾਠ; }
};
class Ast_ਪਾਠ_ਖੰਡ final : ਜਨਤਕ Ast_ਗੰਢ {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ਅਚਲ std::string m_ਅਗੇਤਰ;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ਅਚਲ std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ਅਚਲ std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
ਜਨਤਕ:
    explicit Ast_ਪਾਠ_ਖੰਡ(File_ਸਤਰ* fl,  //
                          ਅਚਲ std::string& ਅਗੇਤਰ = "",  //
                          ਅਚਲ std::string& separator = "",  //
                          ਅਚਲ std::string& suffix = "")
        : ASTGEN_SUPER_ਪਾਠ_ਖੰਡ(fl)
        , m_ਅਗੇਤਰ{ਅਗੇਤਰ}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_Ast_ਪਾਠ_ਖੰਡ;
    ਅਚਲ std::string& ਅਗੇਤਰ() ਅਚਲ { ਵਾਪਸ m_ਅਗੇਤਰ; }
    ਅਚਲ std::string& separator() ਅਚਲ { ਵਾਪਸ m_separator; }
    ਅਚਲ std::string& suffix() ਅਚਲ { ਵਾਪਸ m_suffix; }
    // Add some text, or a node to this block
    ਖਾਲੀ ਜੋੜੋ(ਅਚਲ string& ਪਾਠ) { ਜੋੜੋ_Nodesp(new Ast_ਪਾਠ{fileline(), ਪਾਠ}); }
    ਖਾਲੀ ਜੋੜੋ(Ast_ਗੰਢ* nodep) { ਜੋੜੋ_Nodesp(nodep); }
};
class Ast_ਸਿਖਰ_Scope final : ਜਨਤਕ Ast_ਗੰਢ {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    ਮਿੱਤਰ class AstNetlist;  // Only the AstNetlist can create one
    Ast_ਸਿਖਰ_Scope(File_ਸਤਰ* fl, AstScope* ascopep)
        : ASTGEN_SUPER_ਸਿਖਰ_Scope(fl) {
        this->scopep(ascopep);
    }

ਜਨਤਕ:
    ASTGEN_MEMBERS_Ast_ਸਿਖਰ_Scope;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* create_ਆਰਜ਼ੀ(ਅਚਲ string& name, ਬਿਨਾਨਿਸ਼ਾਨੀ ਚੌੜਾਈ);
    AstVarScope* create_ਆਰਜ਼ੀ(ਅਚਲ string& name, Ast_ਗੰਢ_DType* dtypep);
};
class Ast_Type_ਸਾਰਣੀ final : ਜਨਤਕ Ast_ਗੰਢ {
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
    using DetailedMap = std::map<VBasic_Type_ਕੁੰਜੀ, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

ਜਨਤਕ:
    explicit Ast_Type_ਸਾਰਣੀ(File_ਸਤਰ* fl);
    ASTGEN_MEMBERS_Ast_Type_ਸਾਰਣੀ;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* ਲੱਭੋ_Basic_DType(File_ਸਤਰ* fl, VBasicDTypeKwd kwd);
    AstBasicDType* ਲੱਭੋ_Logic_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(File_ਸਤਰ* fl, VBasicDTypeKwd kwd, ਪੂਰਨ ਚੌੜਾਈ, ਪੂਰਨ ਚੌੜਾਈ_Min,
                                     VSigning numeric);
    AstBasicDType* ਲੱਭੋ_Logic_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(File_ਸਤਰ* fl, VBasicDTypeKwd kwd, ਅਚਲ VNumRange& range,
                                     ਪੂਰਨ ਚੌੜਾਈ_Min, VSigning numeric);
    AstBasicDType* ਲੱਭੋ_Create_Same_DType(AstBasicDType& ਗੰਢ);
    AstBasicDType* ਲੱਭੋ_ਪਾਓ_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* ਲੱਭੋ_Constraint_Ref_DType(File_ਸਤਰ* fl);
    Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType* ਲੱਭੋ_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType(File_ਸਤਰ* fl);
    Ast_ਕਤਾਰ_DType* ਲੱਭੋ_ਕਤਾਰ_ਸੂਚਕਾਂਕ_DType(File_ਸਤਰ* fl);
    Ast_ਧਾਰਾ_DType* ਲੱਭੋ_ਧਾਰਾ_DType(File_ਸਤਰ* fl);
    AstVoidDType* ਲੱਭੋ_Void_DType(File_ਸਤਰ* fl);
    ਖਾਲੀ clearCache();
    ਖਾਲੀ repairCache();
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
};
class AstTypedef final : ਜਨਤਕ Ast_ਗੰਢ {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_ਟੈਗ;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_ਅੰਕ;  // Declaration token number
    bool m_attr_ਜਨਤਕ = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

ਜਨਤਕ:
    AstTypedef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* attrsp, VFlag_ਬੱਚਾ_DType,
               Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ਅੰਕ{fl->token_ਅੰਕ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        ਜੋੜੋ_Attrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ਅੰਕ{fl->token_ਅੰਕ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ਅਚਲ;
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ dtypep() ? dtypep() : ਬੱਚਾ_DTypep();
    }
    // METHODS
    uint32_t decl_Token_ਅੰਕ() ਅਚਲ override { ਵਾਪਸ m_decl_Token_ਅੰਕ; }
    ਖਾਲੀ decl_Token_ਅੰਕ_ਸੈੱਟ_Min(uint32_t token_ਅੰਕ) override {
        m_decl_Token_ਅੰਕ = std::min(m_decl_Token_ਅੰਕ, token_ਅੰਕ);
    }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ name(ਅਚਲ string& flag) override { m_name = flag; }
    bool attr_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_attr_ਜਨਤਕ; }
    ਖਾਲੀ attr_ਜਨਤਕ(bool flag) { m_attr_ਜਨਤਕ = flag; }
    bool isHideLocal() ਅਚਲ { ਵਾਪਸ m_isHideLocal; }
    ਖਾਲੀ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ਅਚਲ { ਵਾਪਸ m_isHideProtected; }
    ਖਾਲੀ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ਅਚਲ { ਵਾਪਸ m_isUnderClass; }
    ਖਾਲੀ isUnderClass(bool flag) { m_isUnderClass = flag; }
    ਖਾਲੀ ਟੈਗ(ਅਚਲ string& ਪਾਠ) override { m_ਟੈਗ = ਪਾਠ; }
    string ਟੈਗ() ਅਚਲ override { ਵਾਪਸ m_ਟੈਗ; }
};
class AstTypedefFwd final : ਜਨਤਕ Ast_ਗੰਢ {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ਅਚਲ VFwdType m_fwdType;  // Forward type for lint check

ਜਨਤਕ:
    AstTypedefFwd(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    VFwdType fwdType() ਅਚਲ { ਵਾਪਸ m_fwdType; }
};
class Ast_Udp_ਸਾਰਣੀ final : ਜਨਤਕ Ast_ਗੰਢ {
    // @astgen op1 := linesp : List[AstUdpTableLine]
ਜਨਤਕ:
    Ast_Udp_ਸਾਰਣੀ(File_ਸਤਰ* fl, Ast_Udp_ਸਾਰਣੀ_ਸਤਰ* linesp)
        : ASTGEN_SUPER_Udp_ਸਾਰਣੀ(fl) {
        this->ਜੋੜੋ_Linesp(linesp);
        ਜੇ (!v3Global.has_ਸਾਰਣੀ()) v3Global.ਸੈੱਟ_Has_ਸਾਰਣੀ();
    }
    ASTGEN_MEMBERS_Ast_Udp_ਸਾਰਣੀ;
};
class Ast_Udp_ਸਾਰਣੀ_ਸਤਰ final : ਜਨਤਕ Ast_ਗੰਢ {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
ਨਿਜੀ:
    ਅਚਲ bool m_udpIsCombo;  // Combinational or sequential UDP

ਜਨਤਕ:
    class UdpCombo {};
    Ast_Udp_ਸਾਰਣੀ_ਸਤਰ(UdpCombo, File_ਸਤਰ* fl, Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val* iFieldsp,
                    Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_ਸਾਰਣੀ_ਸਤਰ(fl)
        , m_udpIsCombo{true} {
        ਜੋੜੋ_IFieldsp(iFieldsp);
        ਜੋੜੋ_OFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_ਸਾਰਣੀ_ਸਤਰ(UdpSequential, File_ਸਤਰ* fl, Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val* iFieldsp,
                    Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val* oFieldsp1, Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_ਸਾਰਣੀ_ਸਤਰ(fl)
        , m_udpIsCombo{false} {
        ਜੋੜੋ_IFieldsp(iFieldsp);
        ਜੋੜੋ_OFieldsp(oFieldsp1);
        ਜੋੜੋ_OFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_ਸਾਰਣੀ_ਸਤਰ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ udpIsCombo() ਅਚਲ { ਵਾਪਸ m_udpIsCombo; }
};
class Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val final : ਜਨਤਕ Ast_ਗੰਢ {
    string m_ਪਾਠ;  // Value character

ਜਨਤਕ:
    Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val(File_ਸਤਰ* fl, ਅਚਲ string& ਪਾਠ)
        : ASTGEN_SUPER_Udp_ਸਾਰਣੀ_ਸਤਰ_Val(fl)
        , m_ਪਾਠ{ਪਾਠ} {}
    ASTGEN_MEMBERS_Ast_Udp_ਸਾਰਣੀ_ਸਤਰ_Val;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_ਪਾਠ; }
    ਖਾਲੀ name(std::string ਅਚਲ& ਪਾਠ) override VL_MT_STABLE { m_ਪਾਠ = ਪਾਠ; }
    string ਪਾਠ() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ਪਾਠ; }
};
class AstVar final : ਜਨਤਕ Ast_ਗੰਢ {
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
    string m_ਟੈਗ;  // Holds the string of the verilator tag -- used in JSON output.
    VVarType m_varType;  // Type of variable
    VDirection m_direction;  // Direction input/output etc
    VDirection m_declDirection;  // Declared direction input/output etc
    VLifetime m_lifetime;  // Lifetime
    VRandAttr m_rand;  // Randomizability of this variable (rand, randc, etc)
    ਪੂਰਨ m_pin_ਅੰਕ = 0;  // For JSON, if non-zero the connection pin number
    ਪੂਰਨ m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
    bool m_libInoutEnable : 1;  // Port is the lowered library inout's enable
    bool m_ansi : 1;  // Params or pins declared in the module header, rather than the body
    bool m_declTyped : 1;  // Declared as type (for dedup check)
    bool m_tristate : 1;  // Inout or triwire or trireg
    bool m_primaryIO : 1;  // In/out to top level (or directly assigned from same)
    bool m_primaryClock : 1;  // In/out to top level and is, or combinationally drives a SenItem
    bool m_sc : 1;  // SystemC variable
    bool m_scSensitive : 1;  // SystemC sensitive() needed
    bool m_sig_ਜਨਤਕ : 1;  // User C code accesses this signal or is top signal
    bool m_sig_Mod_ਜਨਤਕ : 1;  // User C code accesses this signal and module
    bool m_sig_User_Rd_ਜਨਤਕ : 1;  // User C code accesses this signal, read only
    bool m_sigUserRWPublic : 1;  // User C code accesses this signal, read-write
    bool m_ਵਰਤਿਆ_Param : 1;  // Parameter is referenced (on link; later signals not setup)
    bool m_ਵਰਤਿਆ_Loop_Idx : 1;  // Variable subject of for unrolling
    bool m_funcLocal : 1;  // Local variable for a function
    bool m_funcLocalSticky : 1;  // As m_funcLocal but remains set if var is moved to a static
    bool m_funcReturn : 1;  // Return variable for a function
    bool m_attrScBv : 1;  // User force bit vector attribute
    bool m_attrScBigUint : 1;  // User force sc_biguint attribute
    bool m_attrSFormat : 1;  // User sformat attribute
    bool m_attr_ਵੰਡੋ_Var : 1;  // declared with split_var metacomment
    bool m_attrFsmState : 1;  // declared with fsm_state metacomment
    bool m_attrFsmArcInclCond : 1;  // declared with fsm_arc_include_cond metacomment
    VFsmExpandType::en m_attrFsmStateExpand : 2;  // FSM expansion type for FSM coverage
    bool m_const_Pool_ਇੰਦਰਾਜ : 1;  // Constant pool variable
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
    bool m_is_Iface_ਮਾਪਾ : 1;  // dtype is reference to interface present in this module
    bool m_is_Iface_Array_ਵੰਡੋ : 1;  // Interface array that was split into element variables by
                                   // V3Param, until V3Width removes it
    bool m_is_ਅੰਦਰੂਨੀ : 1;  // Internal state, don't add to method pinter
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
    bool m_is_Forced_By_ਕੋਡ : 1;  // May be forced/released from AstAssignForce/AstRelease
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
    bool m_process_ਕਤਾਰ : 1;  // Process queue variable
    bool m_mtask_Cache_ਸਤਰ_Align : 1;  // Start MTask affinity group on a cache line
    ਖਾਲੀ init() {
        m_libInoutEnable = false;
        m_ansi = false;
        m_declTyped = false;
        m_tristate = false;
        m_primaryIO = false;
        m_primaryClock = false;
        m_sc = false;
        m_scSensitive = false;
        m_ਵਰਤਿਆ_Param = false;
        m_ਵਰਤਿਆ_Loop_Idx = false;
        m_sig_ਜਨਤਕ = false;
        m_sig_Mod_ਜਨਤਕ = false;
        m_sig_User_Rd_ਜਨਤਕ = false;
        m_sigUserRWPublic = false;
        m_funcLocal = false;
        m_funcLocalSticky = false;
        m_funcReturn = false;
        m_attrScBv = false;
        m_attrScBigUint = false;
        m_attrSFormat = false;
        m_attr_ਵੰਡੋ_Var = false;
        m_attrFsmState = false;
        m_attrFsmArcInclCond = false;
        m_attrFsmStateExpand = VFsmExpandType::DEFAULT;
        m_const_Pool_ਇੰਦਰਾਜ = false;
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
        m_is_Iface_ਮਾਪਾ = false;
        m_is_Iface_Array_ਵੰਡੋ = false;
        m_is_ਅੰਦਰੂਨੀ = false;
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
        m_is_Forced_By_ਕੋਡ = false;
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
        m_process_ਕਤਾਰ = false;
        m_mtask_Cache_ਸਤਰ_Align = false;
    }

ਜਨਤਕ:
    AstVar(File_ਸਤਰ* fl, VVarType type, ਅਚਲ string& name, VFlag_ਬੱਚਾ_DType, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ਬੱਚਾ_DTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_ਸਤਰ* fl, VVarType type, ਅਚਲ string& name, Ast_ਗੰਢ_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_ਸਤਰ* fl, VVarType type, ਅਚਲ string& name, VFlagLogicPacked, ਪੂਰਨ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_ਸੈੱਟ_Logic_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ਸਤਰ* fl, VVarType type, ਅਚਲ string& name, VFlag_ਦੋਆਧਾਰੀ_ਅੰਕ_Packed, ਪੂਰਨ wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ਸਤਰ* fl, VVarType type, ਅਚਲ string& name, ਅਚਲ AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ਜੇ (examplep->ਬੱਚਾ_DTypep()) ਬੱਚਾ_DTypep(examplep->ਬੱਚਾ_DTypep()->clone_ਰੁੱਖ(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਸਤਰਅੰਦਰ bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }  // * = Var name
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    string origName() ਅਚਲ override { ਵਾਪਸ m_origName; }  // * = Original name
    ਖਾਲੀ origName(ਅਚਲ string& name) { m_origName = name; }
    VVarType varType() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_varType; }  // * = Type of variable
    ਖਾਲੀ direction(ਅਚਲ VDirection& flag) {
        m_direction = flag;
        ਜੇ (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_direction; }
    bool isIO() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_direction != VDirection::NONE; }
    bool isVLIO() ਅਚਲ {
        ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = basicp();
        ਵਾਪਸ isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    ਖਾਲੀ declDirection(ਅਚਲ VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ਅਚਲ { ਵਾਪਸ m_declDirection; }
    ਪੂਰਨ libInoutId() ਅਚਲ { ਵਾਪਸ m_libInoutId; }
    ਖਾਲੀ libInoutId(ਪੂਰਨ id) { m_libInoutId = id; }
    bool libInoutEnable() ਅਚਲ { ਵਾਪਸ m_libInoutEnable; }
    ਖਾਲੀ libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    ਖਾਲੀ varType(VVarType type) { m_varType = type; }
    ਖਾਲੀ var_Type2_ਬਾਹਰਲਾ() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    ਖਾਲੀ var_Type2_ਵਿੱਚ() {
        m_tristate = false;
        m_direction = VDirection::ਨਿਵੇਸ਼;
    }
    string scType() ਅਚਲ;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) ਅਚਲ;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) ਅਚਲ;
    string dpiTmpVarType(ਅਚਲ string& varName) ਅਚਲ;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) ਅਚਲ;
    string vlEnumType() ਅਚਲ;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) ਅਚਲ;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(ਅਚਲ string& propName) ਅਚਲ;  // Return VerilatorVarProps declaration
    ਖਾਲੀ combineType(VVarType type);
    Ast_ਗੰਢ_DType* get_ਬੱਚਾ_DTypep() ਅਚਲ override { ਵਾਪਸ ਬੱਚਾ_DTypep(); }
    Ast_ਗੰਢ_DType* dtypeSkipRefp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ subDTypep()->basicp(); }
    ਆਭਾਸੀ Ast_ਗੰਢ_DType* subDTypep() ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ dtypep() ? dtypep() : ਬੱਚਾ_DTypep();
    }
    ਖਾਲੀ ansi(bool flag) { m_ansi = flag; }
    ਖਾਲੀ declTyped(bool flag) { m_declTyped = flag; }
    ਖਾਲੀ sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    ਖਾਲੀ attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    ਖਾਲੀ attrScBv(bool flag) { m_attrScBv = flag; }
    ਖਾਲੀ attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    ਖਾਲੀ attrSFormat(bool flag) { m_attrSFormat = flag; }
    ਖਾਲੀ attr_ਵੰਡੋ_Var(bool flag) { m_attr_ਵੰਡੋ_Var = flag; }
    ਖਾਲੀ attrFsmState(bool flag) { m_attrFsmState = flag; }
    ਖਾਲੀ attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    ਖਾਲੀ attrFsmStateExpand(ਅਚਲ VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool const_Pool_ਇੰਦਰਾਜ() ਅਚਲ { ਵਾਪਸ m_const_Pool_ਇੰਦਰਾਜ; }
    ਖਾਲੀ ਸੈੱਟ_Const_Pool_ਇੰਦਰਾਜ() { m_const_Pool_ਇੰਦਰਾਜ = true; }
    bool covergroupRefMember() ਅਚਲ { ਵਾਪਸ m_covergroupRefMember; }
    ਖਾਲੀ covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ਅਚਲ { ਵਾਪਸ m_embeddedCovergroup; }
    ਖਾਲੀ embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    ਖਾਲੀ rand(ਅਚਲ VRandAttr flag) { m_rand = flag; }
    ਖਾਲੀ ਵਰਤਿਆ_Param(bool flag) { m_ਵਰਤਿਆ_Param = flag; }
    ਖਾਲੀ ਵਰਤਿਆ_Loop_Idx(bool flag) { m_ਵਰਤਿਆ_Loop_Idx = flag; }
    ਖਾਲੀ sig_ਜਨਤਕ(bool flag) { m_sig_ਜਨਤਕ = flag; }
    ਖਾਲੀ sig_Mod_ਜਨਤਕ(bool flag) { m_sig_Mod_ਜਨਤਕ = flag; }
    ਖਾਲੀ sig_User_Rd_ਜਨਤਕ(bool flag) {
        m_sig_User_Rd_ਜਨਤਕ = flag;
        ਜੇ (flag) sig_ਜਨਤਕ(true);
    }
    ਖਾਲੀ sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        ਜੇ (flag) sig_User_Rd_ਜਨਤਕ(true);
    }
    ਖਾਲੀ sc(bool flag) { m_sc = flag; }
    ਖਾਲੀ scSensitive(bool flag) { m_scSensitive = flag; }
    ਖਾਲੀ primaryIO(bool flag) { m_primaryIO = flag; }
    ਖਾਲੀ isConst(bool flag) { m_isConst = flag; }
    ਖਾਲੀ isContinuously(bool flag) { m_isContinuously = flag; }
    ਖਾਲੀ is_Iface_ਮਾਪਾ(bool flag) { m_is_Iface_ਮਾਪਾ = flag; }
    ਖਾਲੀ is_Iface_Array_ਵੰਡੋ(bool flag) { m_is_Iface_Array_ਵੰਡੋ = flag; }
    ਖਾਲੀ is_ਅੰਦਰੂਨੀ(bool flag) { m_is_ਅੰਦਰੂਨੀ = flag; }
    ਖਾਲੀ isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    ਖਾਲੀ funcLocal(bool flag) {
        m_funcLocal = flag;
        ਜੇ (flag) m_funcLocalSticky = true;
    }
    ਖਾਲੀ funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ਅਚਲ { ਵਾਪਸ m_gotNansiType; }
    ਖਾਲੀ gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ਅਚਲ { ਵਾਪਸ m_hasStrengthAssignment; }
    ਖਾਲੀ hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ਅਚਲ { ਵਾਪਸ m_hasUserInit; }
    ਖਾਲੀ hasUserInit(bool flag) { m_hasUserInit = flag; }
    ਖਾਲੀ icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ਅਚਲ { ਵਾਪਸ m_icoMaybeWritten; }
    ਖਾਲੀ isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ਅਚਲ { ਵਾਪਸ m_isClockingDriven; }
    bool isDpiOpenArray() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_isDpiOpenArray; }
    ਖਾਲੀ isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ਅਚਲ { ਵਾਪਸ m_isHideLocal; }
    ਖਾਲੀ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ਅਚਲ { ਵਾਪਸ m_isHideProtected; }
    ਖਾਲੀ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ਅਚਲ { ਵਾਪਸ m_noCReset; }
    ਖਾਲੀ noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ਅਚਲ { ਵਾਪਸ m_noReset; }
    ਖਾਲੀ noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ਅਚਲ { ਵਾਪਸ m_noSubst; }
    ਖਾਲੀ noSubst(bool flag) { m_noSubst = flag; }
    bool process_ਕਤਾਰ() ਅਚਲ { ਵਾਪਸ m_process_ਕਤਾਰ; }
    ਖਾਲੀ process_ਕਤਾਰ(bool flag) { m_process_ਕਤਾਰ = flag; }
    bool sampled() ਅਚਲ { ਵਾਪਸ m_sampled; }
    ਖਾਲੀ sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ਅਚਲ { ਵਾਪਸ m_substConstOnly; }
    ਖਾਲੀ substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ਅਚਲ { ਵਾਪਸ m_overriddenParam; }
    ਖਾਲੀ overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ਅਚਲ { ਵਾਪਸ m_untypedParam; }
    ਖਾਲੀ untypedParam(bool flag) { m_untypedParam = flag; }
    ਖਾਲੀ trace(bool flag) { m_trace = flag; }
    ਖਾਲੀ isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ਅਚਲ { ਵਾਪਸ m_isForceable; }
    ਖਾਲੀ ਸੈੱਟ_Forceable() { m_isForceable = true; }
    ਖਾਲੀ ਸੈੱਟ_Forced_By_ਕੋਡ() { m_is_Forced_By_ਕੋਡ = true; }
    bool isForced() ਅਚਲ { ਵਾਪਸ m_isForceable || m_is_Forced_By_ਕੋਡ; }
    bool isReadByDpi() ਅਚਲ { ਵਾਪਸ m_isReadByDpi; }
    ਖਾਲੀ ਸੈੱਟ_Read_By_Dpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ਅਚਲ { ਵਾਪਸ m_isWrittenByDpi; }
    ਖਾਲੀ ਸੈੱਟ_Written_By_Dpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ਅਚਲ { ਵਾਪਸ m_isWrittenBySuspendable; }
    ਖਾਲੀ ਸੈੱਟ_Written_By_Suspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ਅਚਲ { ਵਾਪਸ m_ignorePostRead; }
    ਖਾਲੀ ਸੈੱਟ_Ignore_Post_Read() { m_ignorePostRead = true; }
    bool ignorePostWrite() ਅਚਲ { ਵਾਪਸ m_ignorePostWrite; }
    ਖਾਲੀ ਸੈੱਟ_Ignore_Post_Write() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ਅਚਲ { ਵਾਪਸ m_ignoreSchedWrite; }
    ਖਾਲੀ ਸੈੱਟ_Ignore_Sched_Write() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ਅਚਲ { ਵਾਪਸ m_dfgTriLowered; }
    ਖਾਲੀ ਸੈੱਟ_Dfg_Tri_Lowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ਅਚਲ { ਵਾਪਸ m_dfgAllowMultidriveTri; }
    ਖਾਲੀ ਸੈੱਟ_Dfg_Allow_Multidrive_Tri() { m_dfgAllowMultidriveTri = true; }
    ਖਾਲੀ globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ਅਚਲ { ਵਾਪਸ m_globalConstrained; }
    bool isStdRandomizeArg() ਅਚਲ { ਵਾਪਸ m_isStdRandomizeArg; }
    ਖਾਲੀ ਸੈੱਟ_Std_Randomize_Arg() { m_isStdRandomizeArg = true; }
    // METHODS
    ਖਾਲੀ name(ਅਚਲ string& name) override { m_name = name; }
    ਖਾਲੀ ਟੈਗ(ਅਚਲ string& ਪਾਠ) override { m_ਟੈਗ = ਪਾਠ; }
    string ਟੈਗ() ਅਚਲ override { ਵਾਪਸ m_ਟੈਗ; }
    bool isAnsi() ਅਚਲ { ਵਾਪਸ m_ansi; }
    bool isContinuously() ਅਚਲ { ਵਾਪਸ m_isContinuously; }
    bool isDeclTyped() ਅਚਲ { ਵਾਪਸ m_declTyped; }
    bool isInout() ਅਚਲ { ਵਾਪਸ m_direction.isInout(); }
    bool isInoutOrRef() ਅਚਲ { ਵਾਪਸ m_direction.isInoutOrRef(); }
    bool is_ਨਿਵੇਸ਼() ਅਚਲ { ਵਾਪਸ m_direction.is_ਨਿਵੇਸ਼(); }
    bool isNonOutput() ਅਚਲ { ਵਾਪਸ m_direction.isNonOutput(); }
    bool isReadOnly() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_direction.isReadOnly(); }
    bool isConstRef() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_direction.isConstRef(); }
    bool isRef() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_direction.isRef(); }
    bool isWritable() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_direction.isWritable(); }
    bool isTristate() ਅਚਲ { ਵਾਪਸ m_tristate; }
    bool isPrimaryIO() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_primaryIO; }
    bool isPrimaryInish() ਅਚਲ { ਵਾਪਸ isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_primaryClock; }
    ਖਾਲੀ ਸੈੱਟ_Primary_Clock() { m_primaryClock = true; }
    bool isIfaceRef() ਅਚਲ { ਵਾਪਸ varType() == VVarType::IFACEREF; }
    ਖਾਲੀ ਸੈੱਟ_Iface_Ref() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool is_Iface_ਮਾਪਾ() ਅਚਲ { ਵਾਪਸ m_is_Iface_ਮਾਪਾ; }
    bool is_Iface_Array_ਵੰਡੋ() ਅਚਲ { ਵਾਪਸ m_is_Iface_Array_ਵੰਡੋ; }
    bool is_ਅੰਦਰੂਨੀ() ਅਚਲ { ਵਾਪਸ m_is_ਅੰਦਰੂਨੀ; }
    bool isIfaceParam() ਅਚਲ { ਵਾਪਸ m_isIfaceParam; }
    bool isSignal() ਅਚਲ { ਵਾਪਸ varType().isSignal(); }
    bool isNet() ਅਚਲ { ਵਾਪਸ varType().isNet(); }
    bool isWor() ਅਚਲ { ਵਾਪਸ varType().isWor(); }
    bool isWiredNet() ਅਚਲ { ਵਾਪਸ varType().isWiredNet(); }
    bool is_ਆਰਜ਼ੀ() ਅਚਲ { ਵਾਪਸ varType().is_ਆਰਜ਼ੀ(); }
    bool isClassMember() ਅਚਲ { ਵਾਪਸ varType() == VVarType::MEMBER; }
    bool isVirtIface() ਅਚਲ {
        ਜੇ (AstIfaceRefDType* ਅਚਲ dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            ਵਾਪਸ dtp->isVirtual();
        }
        ਵਾਪਸ false;
    }
    bool is_Statement_ਆਰਜ਼ੀ() ਅਚਲ { ਵਾਪਸ varType() == VVarType::STMTTEMP; }
    bool isXTemp() ਅਚਲ { ਵਾਪਸ varType() == VVarType::XTEMP; }
    bool isParam() ਅਚਲ { ਵਾਪਸ varType().isParam(); }
    bool isGParam() ਅਚਲ { ਵਾਪਸ varType() == VVarType::GPARAM; }
    bool isGenVar() ਅਚਲ { ਵਾਪਸ varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ਅਚਲ {
        ਵਾਪਸ isIO() || isSignal() || isClassMember() || is_ਆਰਜ਼ੀ() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ਅਚਲ {
        ਵਾਪਸ !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic() ਅਚਲ {
        ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = basicp();
        ਵਾਪਸ bdtypep && bdtypep->is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic();
    }
    bool is_ਵਰਤਿਆ_Param() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਿਆ_Param; }
    bool is_ਵਰਤਿਆ_Loop_Idx() ਅਚਲ { ਵਾਪਸ m_ਵਰਤਿਆ_Loop_Idx; }
    bool mtask_Cache_ਸਤਰ_Align() ਅਚਲ { ਵਾਪਸ m_mtask_Cache_ਸਤਰ_Align; }
    ਖਾਲੀ mtask_Cache_ਸਤਰ_Align(bool flag) { m_mtask_Cache_ਸਤਰ_Align = flag; }
    bool isSc() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_sc; }
    bool isScQuad() ਅਚਲ;
    bool isScBv() ਅਚਲ VL_MT_STABLE;
    bool isScUint() ਅਚਲ;
    bool isScUintBool() ਅਚਲ;
    bool isScBigUint() ਅਚਲ VL_MT_STABLE;
    bool isScSensitive() ਅਚਲ { ਵਾਪਸ m_scSensitive; }
    bool is_Sig_ਜਨਤਕ() ਅਚਲ;
    bool is_Sig_Mod_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_sig_Mod_ਜਨਤਕ && !isIfaceRef(); }
    bool is_Sig_User_Rd_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_sig_User_Rd_ਜਨਤਕ && !isIfaceRef(); }
    bool isSigUserRWPublic() ਅਚਲ { ਵਾਪਸ m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ਅਚਲ { ਵਾਪਸ m_trace; }
    bool isRand() ਅਚਲ { ਵਾਪਸ m_rand.isRand(); }
    bool isRandC() ਅਚਲ { ਵਾਪਸ m_rand.isRandC(); }
    bool isConst() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_isConst; }
    bool isLatched() ਅਚਲ { ਵਾਪਸ m_isLatched; }
    bool isFuncLocal() ਅਚਲ { ਵਾਪਸ m_funcLocal; }
    bool isFuncLocalSticky() ਅਚਲ { ਵਾਪਸ m_funcLocalSticky; }
    bool isFuncReturn() ਅਚਲ { ਵਾਪਸ m_funcReturn; }
    bool isPullup() ਅਚਲ { ਵਾਪਸ m_isPullup; }
    bool isPulldown() ਅਚਲ { ਵਾਪਸ m_isPulldown; }
    bool attrScBv() ਅਚਲ { ਵਾਪਸ m_attrScBv; }
    bool attrScBigUint() ਅਚਲ { ਵਾਪਸ m_attrScBigUint; }
    bool attrFileDescr() ਅਚਲ { ਵਾਪਸ m_attrFileDescr; }
    bool attrSFormat() ਅਚਲ { ਵਾਪਸ m_attrSFormat; }
    bool attr_ਵੰਡੋ_Var() ਅਚਲ { ਵਾਪਸ m_attr_ਵੰਡੋ_Var; }
    bool attrFsmState() ਅਚਲ { ਵਾਪਸ m_attrFsmState; }
    bool attrFsmArcInclCond() ਅਚਲ { ਵਾਪਸ m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ਅਚਲ { ਵਾਪਸ m_attrFsmStateExpand; }
    AstIface* sensIfacep() ਅਚਲ { ਵਾਪਸ m_sensIfacep; }
    VRandAttr rand() ਅਚਲ { ਵਾਪਸ m_rand; }
    string verilogKwd() ਅਚਲ override;
    ਖਾਲੀ lifetime(ਅਚਲ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ਅਚਲ { ਵਾਪਸ m_lifetime; }
    ਖਾਲੀ pin_ਅੰਕ(ਪੂਰਨ id) { m_pin_ਅੰਕ = id; }
    ਪੂਰਨ pin_ਅੰਕ() ਅਚਲ { ਵਾਪਸ m_pin_ਅੰਕ; }
    ਖਾਲੀ propagateAttrFrom(ਅਚਲ AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        ਜੇ (fromp->attrFileDescr()) attrFileDescr(true);
        ਜੇ (fromp->isContinuously()) isContinuously(true);
    }
    ਖਾਲੀ propagate_ਵੰਡੋ_Attr_From(ਅਚਲ AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attr_ਵੰਡੋ_Var(fromp->attr_ਵੰਡੋ_Var());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ਅਚਲ ਅੱਖਰ* cannot_ਵੰਡੋ_ਕਿਸਮ_Reason() ਅਚਲ;
    ਖਾਲੀ propagateWrapAttrFrom(ਅਚਲ AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    ਖਾਲੀ combineType(ਅਚਲ AstVar* otherp);
    bool needsCReset() ਅਚਲ {
        ਵਾਪਸ !is_Iface_ਮਾਪਾ() && !isIfaceRef() && !noReset() && !isParam() && !is_Statement_ਆਰਜ਼ੀ()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    ਸਥਿਰ AstVar* scVarRecurse(Ast_ਗੰਢ* nodep);
};
class AstVarScope final : ਜਨਤਕ Ast_ਗੰਢ {
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
    bool m_fork_By_ਮੁੱਲ : 1;  // Set before a fork, whose processes get it by value
ਜਨਤਕ:
    AstVarScope(File_ਸਤਰ* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_ਮੁੱਲ = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    ਖਾਲੀ cloneRelink() override {
        ਜੇ (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ scopep()->name() + "->" + varp()->name(); }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    AstVar* varp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_varp; }  // [After Link] Pointer to variable
    ਖਾਲੀ varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_scopep; }  // Pointer to scope it's under
    ਖਾਲੀ scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ਅਚਲ { ਵਾਪਸ m_trace; }
    ਖਾਲੀ trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ਅਚਲ { ਵਾਪਸ m_optimizeLifePost; }
    ਖਾਲੀ optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_ਮੁੱਲ() ਅਚਲ { ਵਾਪਸ m_fork_By_ਮੁੱਲ; }
    ਖਾਲੀ fork_By_ਮੁੱਲ(bool flag) { m_fork_By_ਮੁੱਲ = flag; }
};

// === AstNodeConsItem ===
class Ast_Cons_Array_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_Cons_ਵਸਤੂ {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ਅਚਲ uint64_t m_ਸੂਚਕਾਂਕ;  // Index of the element, with the array's lo() at 0
ਜਨਤਕ:
    Ast_Cons_Array_ਵਸਤੂ(File_ਸਤਰ* fl, uint64_t ਸੂਚਕਾਂਕ, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cons_Array_ਵਸਤੂ(fl, valuep)
        , m_ਸੂਚਕਾਂਕ{ਸੂਚਕਾਂਕ} {}
    ASTGEN_MEMBERS_Ast_Cons_Array_ਵਸਤੂ;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), Cons_Array_ਵਸਤੂ));
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਵਾਪਸ m_ਸੂਚਕਾਂਕ == VN_DBG_AS(samep, Cons_Array_ਵਸਤੂ)->m_ਸੂਚਕਾਂਕ;
    }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }  // By AstConsArray's map
    uint64_t ਸੂਚਕਾਂਕ() ਅਚਲ { ਵਾਪਸ m_ਸੂਚਕਾਂਕ; }
};
class Ast_Cons_Assoc_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_Cons_ਵਸਤੂ {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
ਜਨਤਕ:
    Ast_Cons_Assoc_ਵਸਤੂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* keyp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cons_Assoc_ਵਸਤੂ(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Assoc_ਵਸਤੂ;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), Cons_Assoc_ਵਸਤੂ));
        ਵਾਪਸ nullptr;
    }
};
class Ast_Cons_Dyn_Array_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_Cons_ਵਸਤੂ {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
ਜਨਤਕ:
    Ast_Cons_Dyn_Array_ਵਸਤੂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_ਵਸਤੂ(fl, valuep) {}
    Ast_Cons_Dyn_Array_ਵਸਤੂ(File_ਸਤਰ* fl, AstConst* keyp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cons_Dyn_Array_ਵਸਤੂ(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Dyn_Array_ਵਸਤੂ;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), Cons_Dyn_Array_ਵਸਤੂ));
        ਵਾਪਸ nullptr;
    }
};
class Ast_Cons_ਕਤਾਰ_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_Cons_ਵਸਤੂ {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
ਜਨਤਕ:
    Ast_Cons_ਕਤਾਰ_ਵਸਤੂ(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cons_ਕਤਾਰ_ਵਸਤੂ(fl, valuep) {}
    ASTGEN_MEMBERS_Ast_Cons_ਕਤਾਰ_ਵਸਤੂ;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), Cons_ਕਤਾਰ) && !VN_IS(backp(), Cons_ਕਤਾਰ_ਵਸਤੂ));
        ਵਾਪਸ nullptr;
    }
};
class Ast_Cons_Struct_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_Cons_ਵਸਤੂ {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
ਜਨਤਕ:
    Ast_Cons_Struct_ਵਸਤੂ(File_ਸਤਰ* fl, AstMemberDType* memberp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cons_Struct_ਵਸਤੂ(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_Ast_Cons_Struct_ਵਸਤੂ;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), Cons_Struct_ਵਸਤੂ));
        ਵਾਪਸ nullptr;
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    AstMemberDType* memberp() ਅਚਲ { ਵਾਪਸ m_memberp; }
};
class Ast_Cons_Wildcard_ਵਸਤੂ final : ਜਨਤਕ Ast_ਗੰਢ_Cons_ਵਸਤੂ {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
ਜਨਤਕ:
    Ast_Cons_Wildcard_ਵਸਤੂ(File_ਸਤਰ* fl, AstConst* keyp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep)
        : ASTGEN_SUPER_Cons_Wildcard_ਵਸਤੂ(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_Ast_Cons_Wildcard_ਵਸਤੂ;
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), Cons_Wildcard_ਵਸਤੂ));
        ਵਾਪਸ nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : ਜਨਤਕ Ast_ਗੰਢ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsm_ਮੰਜ਼ਿਲ;  // FSM destination state label
    string m_fsm_ਟੈਗ;  // FSM arc kind tag (e.g. reset, reset_include, default)
    ਪੂਰਨ m_ਵਿੱਥ;  // Offset column numbers to uniq-ify IFs
ਜਨਤਕ:
    AstCoverOtherDecl(File_ਸਤਰ* fl, ਅਚਲ string& ਪੰਨਾ, ਅਚਲ string& comment,
                      ਅਚਲ string& linescov, ਪੂਰਨ ਵਿੱਥ, ਅਚਲ string& fsmVar = "",
                      ਅਚਲ string& fsmFrom = "", ਅਚਲ string& fsm_ਮੰਜ਼ਿਲ = "",
                      ਅਚਲ string& fsm_ਟੈਗ = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, ਪੰਨਾ, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsm_ਮੰਜ਼ਿਲ{fsm_ਮੰਜ਼ਿਲ}
        , m_fsm_ਟੈਗ{fsm_ਟੈਗ}
        , m_ਵਿੱਥ{ਵਿੱਥ} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ ਵਿੱਥ() ਅਚਲ { ਵਾਪਸ m_ਵਿੱਥ; }
    ਪੂਰਨ ਮਾਪ() ਅਚਲ override { ਵਾਪਸ 1; }
    ਅਚਲ string& linescov() ਅਚਲ { ਵਾਪਸ m_linescov; }
    ਅਚਲ string& fsmVar() ਅਚਲ { ਵਾਪਸ m_fsmVar; }
    ਅਚਲ string& fsmFrom() ਅਚਲ { ਵਾਪਸ m_fsmFrom; }
    ਅਚਲ string& fsm_ਮੰਜ਼ਿਲ() ਅਚਲ { ਵਾਪਸ m_fsm_ਮੰਜ਼ਿਲ; }
    ਅਚਲ string& fsm_ਟੈਗ() ਅਚਲ { ਵਾਪਸ m_fsm_ਟੈਗ; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstCoverOtherDecl* ਅਚਲ asamep = VN_DBG_AS(samep, CoverOtherDecl);
        ਵਾਪਸ Super::same_ਗੰਢ(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : ਜਨਤਕ Ast_ਗੰਢ_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ਅਚਲ VNumRange m_range;  // Packed array range covering each toggle bit
ਜਨਤਕ:
    AstCoverToggleDecl(File_ਸਤਰ* fl, ਅਚਲ string& ਪੰਨਾ, ਅਚਲ string& comment,
                       ਅਚਲ VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, ਪੰਨਾ, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਪੂਰਨ ਮਾਪ() ਅਚਲ override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        ਵਾਪਸ 2 * m_range.elements();
    }
    ਅਚਲ VNumRange& range() ਅਚਲ { ਵਾਪਸ m_range; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstCoverToggleDecl* ਅਚਲ asamep = VN_DBG_AS(samep, CoverToggleDecl);
        ਵਾਪਸ Super::same_ਗੰਢ(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : ਜਨਤਕ Ast_ਗੰਢ_FTask {
    // A function inside a module
ਜਨਤਕ:
    AstFunc(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* stmtp, Ast_ਗੰਢ* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    Ast_ਗੰਢ_FTask* cloneType(ਅਚਲ string& name) override {
        ਵਾਪਸ new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "function"; }
};
class AstLet final : ਜਨਤਕ Ast_ਗੰਢ_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
ਜਨਤਕ:
    AstLet(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਅਚਲ ਅੱਖਰ* broken() ਅਚਲ override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_ਵਿਅੰਜਕ));
        ਵਾਪਸ nullptr;
    }
    Ast_ਗੰਢ_FTask* cloneType(ਅਚਲ string& name) override { ਵਾਪਸ new AstLet{fileline(), name}; }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "let"; }
};
class AstProperty final : ਜਨਤਕ Ast_ਗੰਢ_FTask {
    // A property inside a module
ਜਨਤਕ:
    AstProperty(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    Ast_ਗੰਢ_FTask* cloneType(ਅਚਲ string& name) override {
        ਵਾਪਸ new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "property"; }
};
class AstSequence final : ਜਨਤਕ Ast_ਗੰਢ_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
ਜਨਤਕ:
    AstSequence(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool hasDType() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    Ast_ਗੰਢ_FTask* cloneType(ਅਚਲ string& name) override {
        ਵਾਪਸ new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "sequence"; }
    bool isReferenced() ਅਚਲ { ਵਾਪਸ m_referenced; }
    ਖਾਲੀ isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : ਜਨਤਕ Ast_ਗੰਢ_FTask {
    // A task inside a module
ਜਨਤਕ:
    AstTask(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_ਗੰਢ_FTask* cloneType(ਅਚਲ string& name) override {
        ਵਾਪਸ new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "task"; }
};

// === AstNodeFile ===
class AstCFile final : ਜਨਤਕ Ast_ਗੰਢ_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_ਸਰੋਤ : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
ਜਨਤਕ:
    AstCFile(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_ਸਰੋਤ{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
    uint64_t complexityScore() ਅਚਲ { ਵਾਪਸ m_complexityScore; }
    ਖਾਲੀ complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ਅਚਲ { ਵਾਪਸ m_slow; }
    ਖਾਲੀ slow(bool flag) { m_slow = flag; }
    bool ਸਰੋਤ() ਅਚਲ { ਵਾਪਸ m_ਸਰੋਤ; }
    ਖਾਲੀ ਸਰੋਤ(bool flag) { m_ਸਰੋਤ = flag; }
    bool support() ਅਚਲ { ਵਾਪਸ m_support; }
    ਖਾਲੀ support(bool flag) VL_MT_ਸੁਰੱਖਿਅਤ { m_support = flag; }
};
class AstVFile final : ਜਨਤਕ Ast_ਗੰਢ_File {
    // Verilog output file
    // Parents:  NETLIST
ਜਨਤਕ:
    AstVFile(File_ਸਤਰ* fl, ਅਚਲ string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    ਖਾਲੀ dump(std::ostream& str = std::cout) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str = std::cout) ਅਚਲ override;
};
class AstCoverCross final : ਜਨਤਕ Ast_ਗੰਢ_Func_Cov_ਵਸਤੂ {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
ਜਨਤਕ:
    AstCoverCross(File_ਸਤਰ* fl, ਅਚਲ string& name, AstCoverpointRef* itemsp,
                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        ਜੋੜੋ_Itemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class AstCoverpoint final : ਜਨਤਕ Ast_ਗੰਢ_Func_Cov_ਵਸਤੂ {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
ਜਨਤਕ:
    AstCoverpoint(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp,
                  Ast_ਗੰਢ_ਵਿਅੰਜਕ* iffp = nullptr, Ast_ਗੰਢ* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        ਜੋੜੋ_Binsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};

// === AstNodeGen ===
class Ast_Gen_ਖੰਡ final : ਜਨਤਕ Ast_ਗੰਢ_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ਅਚਲ bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ਅਚਲ bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

ਜਨਤਕ:
    Ast_Gen_ਖੰਡ(File_ਸਤਰ* fl, ਅਚਲ string& name, Ast_ਗੰਢ* itemsp, bool implied)
        : ASTGEN_SUPER_Gen_ਖੰਡ(fl)
        , m_name{name}
        , m_unnamed{name.ਖਾਲੀ_ਥਾਵਾਂ()}
        , m_implied{implied} {
        this->ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_Ast_Gen_ਖੰਡ;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    std::string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name; }
    ਖਾਲੀ name(ਅਚਲ std::string& name) override { m_name = name; }
    bool unnamed() ਅਚਲ { ਵਾਪਸ m_unnamed; }
    bool implied() ਅਚਲ { ਵਾਪਸ m_implied; }
    AstDefaultDisable* defaultDisablep() ਅਚਲ { ਵਾਪਸ m_defaultDisablep; }
    ਖਾਲੀ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ਅਚਲ string& outer) ਅਚਲ;
};
class AstGenCase final : ਜਨਤਕ Ast_ਗੰਢ_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
ਜਨਤਕ:
    AstGenCase(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_Gen_Case_ਵਸਤੂ* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : ਜਨਤਕ Ast_ਗੰਢ_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
ਜਨਤਕ:
    AstGenFor(File_ਸਤਰ* fl, Ast_ਗੰਢ* initsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ* incsp, Ast_ਗੰਢ* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->ਜੋੜੋ_Initsp(initsp);
        this->condp(condp);
        this->ਜੋੜੋ_Incsp(incsp);
        this->ਜੋੜੋ_Itemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : ਜਨਤਕ Ast_ਗੰਢ_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
ਜਨਤਕ:
    AstGenIf(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* condp, Ast_ਗੰਢ* thensp, Ast_ਗੰਢ* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->ਜੋੜੋ_Thensp(thensp);
        this->ਜੋੜੋ_Elsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : ਜਨਤਕ Ast_ਗੰਢ_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtype_Name_ਭਰਿਆ;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_ਅੰਕ;  // Declaration token number
    VBaseOverride m_ਅਧਾਰ_Override;  // BaseOverride (inital/final/extends)
    bool m_hasRandVarsUpdate = false;  // Has updateRandVars method,
                                       // which updates pointers to rand variables in clone()
    bool m_covergroup = false;  // Is covergroup (TODO perhaps make a new Ast node type for CG?)
    bool m_extended = false;  // Is extension or extended by other classes
    bool m_interfaceClass = false;  // Interface class
    bool m_needRNG = false;  // Need RNG, uses srandom/randomize
    bool m_use_Virtual_ਜਨਤਕ = false;  // Subclasses need virtual public as uses interface class
    bool m_virtual = false;  // Virtual class
    bool m_printedFrom = false;  // This class is printed from i.e. is used as format arg.
    // Mutable, as only a recursion guard of the const dtypeName(), so also not dumped
    mutable bool m_dtypeNameBusy = false;  // In dtypeName(), which a parameter may lead back to
    // Covergroup options (when m_covergroup is true)
    ਪੂਰਨ m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool ਭਰਿਆ_2, ਅਚਲ string& scope) ਅਚਲ;
    string dtypeNameScope() ਅਚਲ;  // Prefix of dtypeName(true) for the scope declaring the class

ਜਨਤਕ:
    AstClass(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_ਅੰਕ{fl->token_ਅੰਕ()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ isCovergroup() ? "covergroup" : "class"; }
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ true; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    bool timescaleMatters() ਅਚਲ override { ਵਾਪਸ false; }
    AstClassPackage* classOrPackagep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_classOrPackagep; }
    ਖਾਲੀ classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ਅਚਲ VL_MT_STABLE {
        ਵਾਪਸ m_covergroupEnclosingClassp;
    }
    ਖਾਲੀ covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_ਗੰਢ* membersp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ stmtsp(); }
    ਖਾਲੀ ਜੋੜੋ_Membersp(Ast_ਗੰਢ* nodep) { ਜੋੜੋ_Stmtsp(nodep); }
    bool hasRandVarsUpdate() ਅਚਲ { ਵਾਪਸ m_hasRandVarsUpdate; }
    ਖਾਲੀ hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ਅਚਲ { ਵਾਪਸ m_covergroup; }
    ਖਾਲੀ isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ਅਚਲ { ਵਾਪਸ m_extended; }
    ਖਾਲੀ isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ਅਚਲ { ਵਾਪਸ m_interfaceClass; }
    ਖਾਲੀ isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ਅਚਲ { ਵਾਪਸ m_virtual; }
    ਖਾਲੀ isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ਅਚਲ { ਵਾਪਸ m_needRNG; }
    ਖਾਲੀ needRNG(bool flag) { m_needRNG = flag; }
    bool use_Virtual_ਜਨਤਕ() ਅਚਲ { ਵਾਪਸ m_use_Virtual_ਜਨਤਕ; }
    ਖਾਲੀ use_Virtual_ਜਨਤਕ(bool flag) { m_use_Virtual_ਜਨਤਕ = flag; }
    ਖਾਲੀ markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ਅਚਲ { ਵਾਪਸ m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool ਭਰਿਆ_2) ਅਚਲ;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtype_Name_ਵਿੱਚ(ਅਚਲ string& scope) ਅਚਲ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ਖਾਲੀ dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ਅਚਲ { ਵਾਪਸ !m_dtype_Name_ਭਰਿਆ.ਖਾਲੀ_ਥਾਵਾਂ(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_ਸੁਨੇਹਾ() ਅਚਲ override { ਵਾਪਸ hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    ਸਥਿਰ string embeddedCovergroupTypeName(ਅਚਲ string& name) { ਵਾਪਸ "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ਅਚਲ;
    // Covergroup options accessors
    ਪੂਰਨ cgAutoBinMax() ਅਚਲ { ਵਾਪਸ m_cgAutoBinMax; }
    ਖਾਲੀ cgAutoBinMax(ਪੂਰਨ ਮੁੱਲ) { m_cgAutoBinMax = ਮੁੱਲ; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    ਸਥਿਰ bool isClassExtendedFrom(ਅਚਲ AstClass* refClassp, ਅਚਲ AstClass* ਅਧਾਰ_Classp);
    uint32_t decl_Token_ਅੰਕ() ਅਚਲ override { ਵਾਪਸ m_decl_Token_ਅੰਕ; }
    ਖਾਲੀ decl_Token_ਅੰਕ_ਸੈੱਟ_Min(uint32_t token_ਅੰਕ) override {
        m_decl_Token_ਅੰਕ = std::min(m_decl_Token_ਅੰਕ, token_ਅੰਕ);
    }
    ਖਾਲੀ ਅਧਾਰ_Override(ਅਚਲ VBaseOverride& flag) { m_ਅਧਾਰ_Override = flag; }
    VBaseOverride ਅਧਾਰ_Override() ਅਚਲ { ਵਾਪਸ m_ਅਧਾਰ_Override; }
    // Return the lowest class extended from, or this class
    AstClass* ਅਧਾਰ_Most_Classp();
    ਸਥਿਰ bool is_Cacheable_ਬੱਚਾ(ਅਚਲ Ast_ਗੰਢ* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    ਸਾਂਚਾ <typename T_Callable>
    ਖਾਲੀ foreachMember(ਅਚਲ T_Callable& f) {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_ਗੰਢ*>::ਮੁੱਲ
                && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        ਜੇ (ਅਚਲ AstClassExtends* ਅਚਲ cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        ਲਈ (Ast_ਗੰਢ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (Ast_ਗੰਢ::ਨਿੱਜੀ_Type_Test<T_ਗੰਢ>(stmtp)) f(this, static_cast<T_ਗੰਢ*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    ਸਾਂਚਾ <typename T_Callable>
    bool existsMember(ਅਚਲ T_Callable& p) ਅਚਲ {
        using T_ਗੰਢ = typename Function_Arg_No_ਸੂਚਕ_No_CV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ਅਚਲ AstClass*, ਅਚਲ T_ਗੰਢ*>::ਮੁੱਲ
                && std::is_ਅਧਾਰ_ਦਾ<Ast_ਗੰਢ, T_ਗੰਢ>::ਮੁੱਲ,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        ਜੇ (ਅਚਲ AstClassExtends* ਅਚਲ cextendsp = this->extendsp()) {
            ਜੇ (cextendsp->classp()->existsMember(p)) ਵਾਪਸ true;
        }
        ਲਈ (Ast_ਗੰਢ* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (Ast_ਗੰਢ::ਨਿੱਜੀ_Type_Test<T_ਗੰਢ>(stmtp)) {
                ਜੇ (p(this, static_cast<T_ਗੰਢ*>(stmtp))) ਵਾਪਸ true;
            }
        }
        ਵਾਪਸ false;
    }
};
class AstClassPackage final : ਜਨਤਕ Ast_ਗੰਢ_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
ਜਨਤਕ:
    AstClassPackage(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "classpackage"; }
    bool timescaleMatters() ਅਚਲ override { ਵਾਪਸ false; }
    AstClass* classp() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_classp; }
    ਖਾਲੀ classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : ਜਨਤਕ Ast_ਗੰਢ_Module {
    // An interface declaration
    string m_dtype_Name_ਭਰਿਆ;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
ਜਨਤਕ:
    AstIface(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ਅਚਲ override { ਵਾਪਸ false; }
    bool hasVirtualRef() ਅਚਲ { ਵਾਪਸ m_hasVirtualRef; }
    ਖਾਲੀ ਸੈੱਟ_Has_Virtual_Ref() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool ਭਰਿਆ_2) ਅਚਲ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ਖਾਲੀ dtypeNameFreeze();
};
class AstModule final : ਜਨਤਕ Ast_ਗੰਢ_Module {
    // A module declaration
    ਅਚਲ bool m_isChecker;  // Module represents a checker
    ਅਚਲ bool m_is_ਪ੍ਰੋਗਰਾਮ;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

ਜਨਤਕ:
    class Checker {};  // for constructor type-overload selection
    class ਪ੍ਰੋਗਰਾਮ {};  // for constructor type-overload selection
    AstModule(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_ਪ੍ਰੋਗਰਾਮ{false} {}
    AstModule(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_is_ਪ੍ਰੋਗਰਾਮ{false} {}
    AstModule(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname, ਪ੍ਰੋਗਰਾਮ)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_is_ਪ੍ਰੋਗਰਾਮ{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ਅਚਲ override {
        ਵਾਪਸ m_isChecker ? "checker" : m_is_ਪ੍ਰੋਗਰਾਮ ? "program" : "module";
    }
    bool timescaleMatters() ਅਚਲ override { ਵਾਪਸ true; }
    bool isChecker() ਅਚਲ { ਵਾਪਸ m_isChecker; }
    bool is_ਪ੍ਰੋਗਰਾਮ() ਅਚਲ { ਵਾਪਸ m_is_ਪ੍ਰੋਗਰਾਮ; }
    bool hasGenericIface() ਅਚਲ { ਵਾਪਸ m_hasGenericIface; }
    ਖਾਲੀ hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
};
class Ast_Not_ਮਿਲਿਆ_Module final : ਜਨਤਕ Ast_ਗੰਢ_Module {
    // A missing module declaration
ਜਨਤਕ:
    Ast_Not_ਮਿਲਿਆ_Module(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : ASTGEN_SUPER_Not_ਮਿਲਿਆ_Module(fl, name, libname) {}
    ASTGEN_MEMBERS_Ast_Not_ਮਿਲਿਆ_Module;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "/*not-found-*/ module"; }
    bool timescaleMatters() ਅਚਲ override { ਵਾਪਸ false; }
};
class AstPackage final : ਜਨਤਕ Ast_ਗੰਢ_Module {
    // A package declaration
ਜਨਤਕ:
    AstPackage(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "package"; }
    bool timescaleMatters() ਅਚਲ override { ਵਾਪਸ !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : ਜਨਤਕ Ast_ਗੰਢ_Module {
    // A primitive declaration
ਜਨਤਕ:
    AstPrimitive(File_ਸਤਰ* fl, ਅਚਲ string& name, ਅਚਲ string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ਅਚਲ override { ਵਾਪਸ "primitive"; }
    bool timescaleMatters() ਅਚਲ override { ਵਾਪਸ false; }
};

// === AstNodeProcedure ===
class AstAlways final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ਅਚਲ VAlwaysKwd m_keyword;

ਜਨਤਕ:
    AstAlways(File_ਸਤਰ* fl, VAlwaysKwd keyword, Ast_Sen_ਰੁੱਖ* sentreep, Ast_ਗੰਢ* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit ਸਤਰਅੰਦਰ AstAlways(Ast_ਸੌਂਪੋ_W* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    VAlwaysKwd keyword() ਅਚਲ { ਵਾਪਸ m_keyword; }
};
class AstAlwaysObserved final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

ਜਨਤਕ:
    AstAlwaysObserved(File_ਸਤਰ* fl, Ast_Sen_ਰੁੱਖ* sentreep, Ast_ਗੰਢ* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

ਜਨਤਕ:
    explicit AstAlwaysPost(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // Like always but Postponed scheduling region

ਜਨਤਕ:
    AstAlwaysPostponed(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

ਜਨਤਕ:
    explicit AstAlwaysPre(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

ਜਨਤਕ:
    AstAlwaysReactive(File_ਸਤਰ* fl, Ast_Sen_ਰੁੱਖ* sentreep, Ast_ਗੰਢ* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
ਜਨਤਕ:
    AstFinal(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
ਜਨਤਕ:
    AstInitial(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
ਜਨਤਕ:
    AstInitialAutomatic(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : ਜਨਤਕ Ast_ਗੰਢ_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
ਜਨਤਕ:
    AstInitialStatic(File_ਸਤਰ* fl, Ast_ਗੰਢ* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : ਜਨਤਕ Ast_ਗੰਢ_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
ਜਨਤਕ:
    AstBracketRange(File_ਸਤਰ* fl, Ast_ਗੰਢ* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    ਆਭਾਸੀ string emitC() { V3ERROR_NA_RETURN(""); }
    ਆਭਾਸੀ string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybe_Pointed_ਮੰਜ਼ਿਲ() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ false; }
};
class AstRange final : ਜਨਤਕ Ast_ਗੰਢ_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ਅਚਲ bool m_fromBracket = false;  // From C-style '[x]' declaration
ਜਨਤਕ:
    AstRange(File_ਸਤਰ* fl, Ast_ਗੰਢ_ਵਿਅੰਜਕ* leftp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    ਸਤਰਅੰਦਰ AstRange(File_ਸਤਰ* fl, ਪੂਰਨ ਖੱਬਾ, ਪੂਰਨ ਸੱਜਾ);
    ਸਤਰਅੰਦਰ AstRange(File_ਸਤਰ* fl, ਅਚਲ VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    ਸਤਰਅੰਦਰ ਪੂਰਨ ਖੱਬਾ_Const() ਅਚਲ VL_MT_STABLE;
    ਸਤਰਅੰਦਰ ਪੂਰਨ ਸੱਜਾ_Const() ਅਚਲ VL_MT_STABLE;
    ਪੂਰਨ hiConst() ਅਚਲ VL_MT_STABLE {
        ਅਚਲ ਪੂਰਨ l = ਖੱਬਾ_Const();
        ਅਚਲ ਪੂਰਨ r = ਸੱਜਾ_Const();
        ਵਾਪਸ l > r ? l : r;
    }
    ਪੂਰਨ loConst() ਅਚਲ VL_MT_STABLE {
        ਅਚਲ ਪੂਰਨ l = ਖੱਬਾ_Const();
        ਅਚਲ ਪੂਰਨ r = ਸੱਜਾ_Const();
        ਵਾਪਸ l > r ? r : l;
    }
    ਪੂਰਨ elementsConst() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ hiConst() - loConst() + 1; }
    bool ascending() ਅਚਲ { ਵਾਪਸ ਖੱਬਾ_Const() < ਸੱਜਾ_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ() ਅਚਲ {
        ਵਾਪਸ VNumRange{ਖੱਬਾ_Const(), ਸੱਜਾ_Const()}.seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_ਸੱਜਾ_ਮੰਜ਼ਿਲ_ਖੱਬਾ() ਅਚਲ {
        ਵਾਪਸ VNumRange{ਖੱਬਾ_Const(), ਸੱਜਾ_Const()}.seq_ਸੱਜਾ_ਮੰਜ਼ਿਲ_ਖੱਬਾ();
    }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ override;
    ਖਾਲੀ dumpJson(std::ostream& str) ਅਚਲ override;
    ਆਭਾਸੀ string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* samep) ਅਚਲ override {
        ਅਚਲ AstRange* ਅਚਲ asamep = VN_DBG_AS(samep, Range);
        ਵਾਪਸ fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ਅਚਲ { ਵਾਪਸ m_fromBracket; }
};
class AstUnsizedRange final : ਜਨਤਕ Ast_ਗੰਢ_Range {
    // Unsized range specification, for open arrays
ਜਨਤਕ:
    explicit AstUnsizedRange(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    ਆਭਾਸੀ string emitC() { V3ERROR_NA_RETURN(""); }
    ਆਭਾਸੀ string emitVerilog() { ਵਾਪਸ "[]"; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};
class AstWildcardRange final : ਜਨਤਕ Ast_ਗੰਢ_Range {
    // Wildcard range specification, for wildcard index type associative arrays
ਜਨਤਕ:
    explicit AstWildcardRange(File_ਸਤਰ* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    ਆਭਾਸੀ string emitC() { V3ERROR_NA_RETURN(""); }
    ਆਭਾਸੀ string emitVerilog() { ਵਾਪਸ "[*]"; }
    bool same_ਗੰਢ(ਅਚਲ Ast_ਗੰਢ* /*samep*/) ਅਚਲ override { ਵਾਪਸ true; }
};

#ਜੇ_ਅੰਤ  // Guard
