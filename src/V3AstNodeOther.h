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
#ଅନ୍ତର୍ଭାଵ "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h
#define VL_NOT_FINAL  // This #define fixes broken code folding in the CLion IDE
#endif

#ଅନ୍ତର୍ଭାଵ <unordered_map>

// === Abstract base node types (AstNode*) =====================================

class Ast_ଗଣ୍ଠି_Cons_Item VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // An item of an AstNodeCons,
    // op1 used by some subclasses
    // @astgen op2 := valuep : AstNodeExpr
    friend class Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି;  // For m_knownConstantTree
    mutable bool m_known_Constant_ବୃକ୍ଷ = false;  // Known to be a tree of constants

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Cons_Item(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : Ast_ଗଣ୍ଠି{t, fl} {
        this->valuep(valuep);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Cons_Item;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class Ast_ଗଣ୍ଠି_Cover_Decl VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Coverage analysis point declaration
    //
    // [After V3CoverageJoin] Duplicate declaration to get data from instead
    // @astgen ptr := m_dataDeclp : Optional[AstNodeCoverDecl]
    string m_page;  // Coverage point's page tag
    string m_text;  // Coverage point's text
    string m_hier;  // Coverage point's hierarchy
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_bin_ଅଙ୍କ = 0;  // Global coverage bin offset in the symbol table coverage array
    // Coverage counters are emitted in each module object, so duplicate
    // no-inline instances can keep independent counts for forcePerInstance.
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_local_Bin_ଅଙ୍କ = 0;  // Per-module coverage bin offset
    bool m_perInstance = false;  // Don't clone during inlining, already per instance
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Cover_Decl(VNType t, File_ଧାଡ଼ି* fl, ଅଚଳ string& page, ଅଚଳ string& comment)
        : Ast_ଗଣ୍ଠି(t, fl)
        , m_page{page}
        , m_text{comment} {}
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Cover_Decl;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        ଯଦି (m_dataDeclp
            && (m_dataDeclp == this || m_dataDeclp->m_dataDeclp)) {  // Avoid O(n^2) accessing
            v3fatalSrc("dataDeclp should point to real data, not be a list: " << cvtToHex(this));
        }
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 1 + 2 * INSTR_ଗଣନା_LD; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା bin_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_bin_ଅଙ୍କ; }
    ଶୂନ୍ୟ bin_ଅଙ୍କ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା flag) { m_bin_ଅଙ୍କ = flag; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା local_Bin_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_local_Bin_ଅଙ୍କ; }
    ଶୂନ୍ୟ local_Bin_ଅଙ୍କ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା flag) { m_local_Bin_ଅଙ୍କ = flag; }
    bool perInstance() ଅଚଳ { ଫେରନ୍ତୁ m_perInstance; }
    ଶୂନ୍ୟ perInstance(bool flag) { m_perInstance = flag; }
    ଆଭାସୀ ପୂର୍ଣ୍ଣସଂଖ୍ୟା size() ଅଚଳ = 0;
    ଅଚଳ string& comment() ଅଚଳ { ଫେରନ୍ତୁ m_text; }  // text to insert in code
    ଅଚଳ string& page() ଅଚଳ { ଫେରନ୍ତୁ m_page; }
    ଅଚଳ string& hier() ଅଚଳ { ଫେରନ୍ତୁ m_hier; }
    ଶୂନ୍ୟ hier(ଅଚଳ string& flag) { m_hier = flag; }
    ଶୂନ୍ୟ comment(ଅଚଳ string& flag) { m_text = flag; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ Ast_ଗଣ୍ଠି_Cover_Decl* ଅଚଳ asamep = VN_DBG_AS(samep, ଗଣ୍ଠି_Cover_Decl);
        ଫେରନ୍ତୁ (fileline() == asamep->fileline() && hier() == asamep->hier()
                && comment() == asamep->comment() && page() == asamep->page()
                && perInstance() == asamep->perInstance());
    }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    ଶୂନ୍ୟ dataDeclp(Ast_ଗଣ୍ଠି_Cover_Decl* nodep) { m_dataDeclp = nodep; }
    // dataDecl nullptr means "use this one", but often you want "this" to
    // indicate to get data from here
    Ast_ଗଣ୍ଠି_Cover_Decl* dataDeclNullp() ଅଚଳ { ଫେରନ୍ତୁ m_dataDeclp; }
    Ast_ଗଣ୍ଠି_Cover_Decl* dataDeclThisp() { ଫେରନ୍ତୁ dataDeclNullp() ? dataDeclNullp() : this; }
};
class Ast_ଗଣ୍ଠି_FTask VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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

ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_FTask(VNType t, File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* stmtsp)
        : Ast_ଗଣ୍ଠି{t, fl}
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

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_FTask;
    ଆଭାସୀ Ast_ଗଣ୍ଠି_FTask* cloneType(ଅଚଳ string& name) = 0;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool isGateOptimizable() ଅଚଳ override {
        ଫେରନ୍ତୁ !((m_dpiExport || m_dpiImport) && !m_dpiPure);
    }
    // {AstFunc only} op1 = Range output variable
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string cname() ଅଚଳ { ଫେରନ୍ତୁ m_cname; }
    ଶୂନ୍ୟ cname(ଅଚଳ string& cname) { m_cname = cname; }
    bool isFunction() ଅଚଳ { ଫେରନ୍ତୁ fvarp() != nullptr; }
    // MORE ACCESSORS
    ଶୂନ୍ୟ dpiOpenParentInc() { ++m_dpiOpenParent; }
    ଶୂନ୍ୟ dpiOpenParentClear() { m_dpiOpenParent = 0; }
    uint64_t dpiOpenParent() ଅଚଳ { ଫେରନ୍ତୁ m_dpiOpenParent; }
    bool taskPublic() ଅଚଳ { ଫେରନ୍ତୁ m_taskPublic; }
    ଶୂନ୍ୟ taskPublic(bool flag) { m_taskPublic = flag; }
    bool classMethod() ଅଚଳ { ଫେରନ୍ତୁ m_classMethod; }
    ଶୂନ୍ୟ classMethod(bool flag) { m_classMethod = flag; }
    bool didProto() ଅଚଳ { ଫେରନ୍ତୁ m_didProto; }
    ଶୂନ୍ୟ didProto(bool flag) { m_didProto = flag; }
    bool isExternProto() ଅଚଳ { ଫେରନ୍ତୁ m_isExternProto; }
    ଶୂନ୍ୟ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternDef() ଅଚଳ { ଫେରନ୍ତୁ m_isExternDef; }
    ଶୂନ୍ୟ isExternDef(bool flag) { m_isExternDef = flag; }
    ଅଚଳ string& ifacePortName() ଅଚଳ { ଫେରନ୍ତୁ m_ifacePortName; }
    ଶୂନ୍ୟ ifacePortName(ଅଚଳ string& name) { m_ifacePortName = name; }
    bool prototype() ଅଚଳ { ଫେରନ୍ତୁ m_prototype; }
    ଶୂନ୍ୟ prototype(bool flag) { m_prototype = flag; }
    bool dpiExport() ଅଚଳ { ଫେରନ୍ତୁ m_dpiExport; }
    ଶୂନ୍ୟ dpiExport(bool flag) { m_dpiExport = flag; }
    bool dpiImport() ଅଚଳ { ଫେରନ୍ତୁ m_dpiImport; }
    ଶୂନ୍ୟ dpiImport(bool flag) { m_dpiImport = flag; }
    bool dpiContext() ଅଚଳ { ଫେରନ୍ତୁ m_dpiContext; }
    ଶୂନ୍ୟ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiOpenChild() ଅଚଳ { ଫେରନ୍ତୁ m_dpiOpenChild; }
    ଶୂନ୍ୟ dpiOpenChild(bool flag) { m_dpiOpenChild = flag; }
    bool dpiTask() ଅଚଳ { ଫେରନ୍ତୁ m_dpiTask; }
    ଶୂନ୍ୟ dpiTask(bool flag) { m_dpiTask = flag; }
    bool dpiCDeclOverride() ଅଚଳ { ଫେରନ୍ତୁ !m_dpiCDecl.empty(); }
    ଅଚଳ string& dpiCDecl() ଅଚଳ { ଫେରନ୍ତୁ m_dpiCDecl; }
    ଶୂନ୍ୟ dpiCDecl(ଅଚଳ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isConstructor() ଅଚଳ { ଫେରନ୍ତୁ m_isConstructor; }
    ଶୂନ୍ୟ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isHideLocal() ଅଚଳ { ଫେରନ୍ତୁ m_isHideLocal; }
    ଶୂନ୍ୟ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ଅଚଳ { ଫେରନ୍ତୁ m_isHideProtected; }
    ଶୂନ୍ୟ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool dpiPure() ଅଚଳ { ଫେରନ୍ତୁ m_dpiPure; }
    ଶୂନ୍ୟ dpiPure(bool flag) { m_dpiPure = flag; }
    bool keepAlive() ଅଚଳ { ଫେରନ୍ତୁ m_keepAlive; }
    ଶୂନ୍ୟ keepAlive(bool flag) { m_keepAlive = flag; }
    bool needsSyms() ଅଚଳ { ଫେରନ୍ତୁ m_needsSyms; }
    ଶୂନ୍ୟ needsSyms(bool flag) { m_needsSyms = flag; }
    bool pureVirtual() ଅଚଳ { ଫେରନ୍ତୁ m_pureVirtual; }
    ଶୂନ୍ୟ pureVirtual(bool flag) { m_pureVirtual = flag; }
    bool recursive() ଅଚଳ { ଫେରନ୍ତୁ m_recursive; }
    ଶୂନ୍ୟ recursive(bool flag) { m_recursive = flag; }
    bool isStatic() ଅଚଳ { ଫେରନ୍ତୁ m_static; }
    ଶୂନ୍ୟ isStatic(bool flag) { m_static = flag; }
    bool underGenerate() ଅଚଳ { ଫେରନ୍ତୁ m_underGenerate; }
    ଶୂନ୍ୟ underGenerate(bool flag) { m_underGenerate = flag; }
    bool verilogFunction() ଅଚଳ { ଫେରନ୍ତୁ m_verilogFunction; }
    ଶୂନ୍ୟ verilogFunction(bool flag) { m_verilogFunction = flag; }
    bool verilogTask() ଅଚଳ { ଫେରନ୍ତୁ m_verilogTask; }
    ଶୂନ୍ୟ verilogTask(bool flag) { m_verilogTask = flag; }
    bool isVirtual() ଅଚଳ { ଫେରନ୍ତୁ m_virtual; }
    ଶୂନ୍ୟ isVirtual(bool flag) { m_virtual = flag; }
    bool needProcess() ଅଚଳ { ଫେରନ୍ତୁ m_needProcess; }
    ଶୂନ୍ୟ setNeedProcess() { m_needProcess = true; }
    bool isCovergroupSample() ଅଚଳ { ଫେରନ୍ତୁ m_isCovergroupSample; }
    ଶୂନ୍ୟ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    ଶୂନ୍ୟ baseOverride(ଅଚଳ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ଅଚଳ { ଫେରନ୍ତୁ m_baseOverride; }
    ଶୂନ୍ୟ lifetime(ଅଚଳ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ଅଚଳ { ଫେରନ୍ତୁ m_lifetime; }
    bool isPure() override;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ propagateAttrFrom(ଅଚଳ Ast_ଗଣ୍ଠି_FTask* fromp) {
        // Creating a wrapper with e.g. cloneType(); preserve some attributes
        classMethod(fromp->classMethod());
        isHideLocal(fromp->isHideLocal());
        isHideProtected(fromp->isHideProtected());
        isVirtual(fromp->isVirtual());
        isStatic(fromp->isStatic());
        lifetime(fromp->lifetime());
        underGenerate(fromp->underGenerate());
    }

ବ୍ୟକ୍ତିଗତ:
    bool getPurityRecurse() ଅଚଳ;
};
class Ast_ଗଣ୍ଠି_File VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Emitted Output file
    // Parents:  NETLIST
    // @astgen op1 := tblockp : Optional[AstTextBlock]
    string m_name;  ///< Filename
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_File(VNType t, File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : Ast_ଗଣ୍ଠି{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_File;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଣ୍ଠି_Func_Cov_Item VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Base class for functional coverage items (coverpoints, crosses)
ସୁରକ୍ଷିତ:
    string m_name;  // Item name

ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Func_Cov_Item(VNType t, File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : Ast_ଗଣ୍ଠି{t, fl}
        , m_name{name} {}
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Func_Cov_Item;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    bool maybePointedTo() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class Ast_ଗଣ୍ଠି_Gen VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Generate construct
ସର୍ବସାଧାରଣ:
    Ast_ଗଣ୍ଠି_Gen(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି{t, fl} {}
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Gen;
};
class Ast_ଗଣ୍ଠି_Module VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A module, package, program or interface declaration;
    // something that can live directly under the TOP,
    // excluding $unit package stuff
    // @astgen op1 := stmtsp : List[AstNode]
    // @astgen op2 := scSectionsp : List[AstSystemCSection]
    // op3: Reserved, do not use (including in subclasses)
    string m_name;  // Name of the module
    ଅଚଳ string m_origName;  // Name of the module, ignoring name() changes, for dot lookup
    // dist-ast-dump-suppress  // For some user errors messages only, visible where used
    string m_someInstanceName;  // Hierarchical name of some arbitrary instance of this module.
    string m_libname;  // Work library
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_depth = 0;  // 1=top module, 2=cell off top, shared things low, for -depth options
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_level = 0;  // 1=top module, 2=cell off top, shared things have high number
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
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Module(VNType t, File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : Ast_ଗଣ୍ଠି{t, fl}
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

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Module;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଆଭାସୀ bool timescaleMatters() ଅଚଳ = 0;
    ଧାଡ଼ିଭିତରେ bool isDollarUnit() ଅଚଳ;  // Is the $unit package
    ଧାଡ଼ିଭିତରେ bool isConstPool() ଅଚଳ;  // Is the constant pool package
    // Prefix of the names of the types declared within, as $typename names them, given 'outer',
    // the prefix for the scope declaring this, which names only a class, e.g. '$unit::',
    // 'ifc#(8).', '$unit::Cls#(8)::', or 'top.', see VDTypeNameScopes
    string dtypeNameInnerScope(ଅଚଳ string& outer) ଅଚଳ;
    // ACCESSORS
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string origName() ଅଚଳ override { ଫେରନ୍ତୁ m_origName; }
    string someInstanceName() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_someInstanceName; }
    ଶୂନ୍ୟ someInstanceName(ଅଚଳ string& name) { m_someInstanceName = name; }
    AstDefaultDisable* defaultDisablep() ଅଚଳ { ଫେରନ୍ତୁ m_defaultDisablep; }
    ଶୂନ୍ୟ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    bool inLibrary() ଅଚଳ { ଫେରନ୍ତୁ m_inLibrary; }
    ଶୂନ୍ୟ inLibrary(bool flag) { m_inLibrary = flag; }
    ଶୂନ୍ୟ depth(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) { m_depth = ମୂଲ୍ୟ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_depth; }
    ଶୂନ୍ୟ level(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) { m_level = ମୂଲ୍ୟ; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା level() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_level; }
    string libname() ଅଚଳ { ଫେରନ୍ତୁ m_libname; }
    bool is_ଶୀର୍ଷ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ level() == 1; }
    bool modPublic() ଅଚଳ { ଫେରନ୍ତୁ m_modPublic; }
    ଶୂନ୍ୟ modPublic(bool flag) { m_modPublic = flag; }
    bool modTrace() ଅଚଳ { ଫେରନ୍ତୁ m_modTrace; }
    ଶୂନ୍ୟ modTrace(bool flag) { m_modTrace = flag; }
    bool ctorVarReset() ଅଚଳ { ଫେରନ୍ତୁ m_ctorVarReset; }
    ଶୂନ୍ୟ ctorVarReset(bool flag) { m_ctorVarReset = flag; }
    bool dead() ଅଚଳ { ଫେରନ୍ତୁ m_dead; }
    ଶୂନ୍ୟ dead(bool flag) { m_dead = flag; }
    bool hasGParam() ଅଚଳ { ଫେରନ୍ତୁ m_hasGParam; }
    ଶୂନ୍ୟ hasGParam(bool flag) { m_hasGParam = flag; }
    bool hasParameterList() ଅଚଳ { ଫେରନ୍ତୁ m_hasParameterList; }
    ଶୂନ୍ୟ hasParameterList(bool flag) { m_hasParameterList = flag; }
    bool hierBlock() ଅଚଳ { ଫେରନ୍ତୁ m_hierBlock; }
    ଶୂନ୍ୟ hierBlock(bool flag) { m_hierBlock = flag; }
    bool hierParams() ଅଚଳ { ଫେରନ୍ତୁ m_hierParams; }
    ଶୂନ୍ୟ hierParams(bool flag) { m_hierParams = flag; }
    bool recursive() ଅଚଳ { ଫେରନ୍ତୁ m_recursive; }
    ଶୂନ୍ୟ recursive(bool flag) { m_recursive = flag; }
    ଶୂନ୍ୟ recursiveClone(bool flag) { m_recursiveClone = flag; }
    bool recursiveClone() ଅଚଳ { ଫେରନ୍ତୁ m_recursiveClone; }
    bool parameterizedTemplate() ଅଚଳ { ଫେରନ୍ତୁ m_parameterizedTemplate; }
    ଶୂନ୍ୟ parameterizedTemplate(bool flag) { m_parameterizedTemplate = flag; }
    ଶୂନ୍ୟ verilatorLib(bool flag) { m_verilatorLib = flag; }
    bool verilatorLib() ଅଚଳ { ଫେରନ୍ତୁ m_verilatorLib; }
    VLifetime lifetime() ଅଚଳ { ଫେରନ୍ତୁ m_lifetime; }
    ଶୂନ୍ୟ lifetime(ଅଚଳ VLifetime& flag) { m_lifetime = flag; }
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& flag) { m_timeunit = flag; }
    VOptionBool unconnectedDrive() ଅଚଳ { ଫେରନ୍ତୁ m_unconnectedDrive; }
    ଶୂନ୍ୟ unconnectedDrive(ଅଚଳ VOptionBool flag) { m_unconnectedDrive = flag; }
};
class Ast_ଗଣ୍ଠି_Procedure VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // IEEE procedure: initial, final, always
    // @astgen op2 := stmtsp : List[AstNode] // Note: op1 is used in some sub-types only
    bool m_suspendable : 1;  // Is suspendable by a Delay, EventControl, etc.
    bool m_needProcess : 1;  // Uses VlProcess
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Procedure(VNType t, File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : Ast_ଗଣ୍ଠି{t, fl} {
        m_needProcess = false;
        m_suspendable = false;
        addStmtsp(stmtsp);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Procedure;
    // METHODS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isJustOneBodyStmt() ଅଚଳ { ଫେରନ୍ତୁ stmtsp() && !stmtsp()->nextp(); }
    bool isSuspendable() ଅଚଳ { ଫେରନ୍ତୁ m_suspendable; }
    ଶୂନ୍ୟ setSuspendable() { m_suspendable = true; }
    bool needProcess() ଅଚଳ { ଫେରନ୍ତୁ m_needProcess; }
    ଶୂନ୍ୟ setNeedProcess() { m_needProcess = true; }
};
class Ast_ଗଣ୍ଠି_Range VL_NOT_FINAL : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A range, sized or unsized
ସୁରକ୍ଷିତ:
    Ast_ଗଣ୍ଠି_Range(VNType t, File_ଧାଡ଼ି* fl)
        : Ast_ଗଣ୍ଠି{t, fl} {}

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଗଣ୍ଠି_Range;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};

// === Concrete node types =====================================================

// === AstNode ===
class AstActive final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Block of code with sensitivity activation
    // Parents:  MODULE | CFUNC
    // @astgen op1 := senTreeStorep : Optional[AstSenTree] // Moved into m_sensesp in V3Active
    // @astgen op2 := stmtsp : List[AstNode] // Logic
    //
    // @astgen ptr := m_sentreep : Optional[AstSenTree]  // Sensitivity list for this process
    string m_name;

ସର୍ବସାଧାରଣ:
    AstActive(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_Sen_ବୃକ୍ଷ* sentreep)
        : ASTGEN_SUPER_Active(fl)
        , m_name{name}
        , m_sentreep{sentreep} {
        UASSERT(sentreep, "Sentreep required arg");
    }
    ASTGEN_MEMBERS_AstActive;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    // Statements are broken into pieces, as some must come before others.
    ଶୂନ୍ୟ sentreep(Ast_Sen_ବୃକ୍ଷ* nodep) { m_sentreep = nodep; }
    Ast_Sen_ବୃକ୍ଷ* sentreep() ଅଚଳ { ଫେରନ୍ତୁ m_sentreep; }
    // METHODS
    ଧାଡ଼ିଭିତରେ bool hasClocked() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool hasCombo() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool hasInitial() ଅଚଳ;
    ଧାଡ଼ିଭିତରେ bool hasStatic() ଅଚଳ;
};
class AstAlias final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Alias construct - Used for source level net alias, and also for variable aliases internally
    // @astgen op1 := itemsp : List[AstNodeExpr]  // Alias operands
ସର୍ବସାଧାରଣ:
    AstAlias(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemsp)
        : ASTGEN_SUPER_Alias(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstAlias;
};
class AstAliasScope final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Like AstAlias, but aliases two scopes instead of nets/variables
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := lhsp : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstAliasScope(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_AliasScope(fl) {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstAliasScope;
};
class AstBind final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents: MODULE
    // Children: CELL
    // @astgen op1 := cellsp : List[AstNode]
    string m_name;  // Binding to name
ସର୍ବସାଧାରଣ:
    AstBind(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* cellsp)
        : ASTGEN_SUPER_Bind(fl)
        , m_name{name} {
        UASSERT_OBJ(VN_IS(cellsp, Cell), cellsp, "Only instances allowed to be bound");
        addCellsp(cellsp);
    }
    ASTGEN_MEMBERS_AstBind;
    // ACCESSORS
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Bind Target name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
};
class AstCFunc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_cost;  // Function call cost
ସର୍ବସାଧାରଣ:
    AstCFunc(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstScope* scopep, ଅଚଳ string& rtnType = "")
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
        m_cost = v3Global.opt.instr_ଗଣନା_Dpi();  // As proxy for unknown general DPI cost
    }
    ASTGEN_MEMBERS_AstCFunc;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstCFunc* ଅଚଳ asamep = VN_DBG_AS(samep, CFunc);
        ଫେରନ୍ତୁ ((isTrace() == asamep->isTrace()) && (rtnTypeVoid() == asamep->rtnTypeVoid())
                && (argTypes() == asamep->argTypes()) && isLoose() == asamep->isLoose()
                && voidSelf() == asamep->voidSelf()
                && (!(dpiImportPrototype() || dpiExportImpl()) || name() == asamep->name()));
    }
    //
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ m_cost; }
    VBoolOrUnknown isConst() ଅଚଳ { ଫେରନ୍ତୁ m_isConst; }
    ଶୂନ୍ୟ isConst(bool flag) { m_isConst.setTrueOrFalse(flag); }
    ଶୂନ୍ୟ isConst(VBoolOrUnknown flag) { m_isConst = flag; }
    bool isStatic() ଅଚଳ { ଫେରନ୍ତୁ m_isStatic; }
    ଶୂନ୍ୟ isStatic(bool flag) { m_isStatic = flag; }
    bool isTrace() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_isTrace; }
    ଶୂନ୍ୟ isTrace(bool flag) { m_isTrace = flag; }
    ଶୂନ୍ୟ cname(ଅଚଳ string& name) { m_cname = name; }
    string cname() ଅଚଳ { ଫେରନ୍ତୁ m_cname; }
    AstScope* scopep() ଅଚଳ { ଫେରନ୍ତୁ m_scopep; }
    ଶୂନ୍ୟ scopep(AstScope* nodep) { m_scopep = nodep; }
    string rtnTypeVoid() ଅଚଳ { ଫେରନ୍ତୁ ((m_rtnType == "") ? "void" : m_rtnType); }
    ଶୂନ୍ୟ rtnType(ଅଚଳ string& rtnType) { m_rtnType = rtnType; }
    bool dontCombine() ଅଚଳ { ଫେରନ୍ତୁ m_dontCombine || isTrace() || entryPoint(); }
    ଶୂନ୍ୟ dontCombine(bool flag) { m_dontCombine = flag; }
    bool declPrivate() ଅଚଳ { ଫେରନ୍ତୁ m_declPrivate; }
    ଶୂନ୍ୟ declPrivate(bool flag) { m_declPrivate = flag; }
    bool keepIfEmpty() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_keepIfEmpty; }
    ଶୂନ୍ୟ keepIfEmpty(bool flag) { m_keepIfEmpty = flag; }
    bool slow() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_slow; }
    ଶୂନ୍ୟ slow(bool flag) { m_slow = flag; }
    bool funcPublic() ଅଚଳ { ଫେରନ୍ତୁ m_funcPublic; }
    ଶୂନ୍ୟ funcPublic(bool flag) { m_funcPublic = flag; }
    ଶୂନ୍ୟ argTypes(ଅଚଳ string& str) { m_argTypes = str; }
    string argTypes() ଅଚଳ { ଫେରନ୍ତୁ m_argTypes; }
    ଶୂନ୍ୟ ifdef(ଅଚଳ string& str) { m_ifdef = str; }
    string ifdef() ଅଚଳ { ଫେରନ୍ତୁ m_ifdef; }
    bool isConstructor() ଅଚଳ { ଫେରନ୍ତୁ m_isConstructor; }
    ଶୂନ୍ୟ isConstructor(bool flag) { m_isConstructor = flag; }
    bool isDestructor() ଅଚଳ { ଫେରନ୍ତୁ m_isDestructor; }
    ଶୂନ୍ୟ isDestructor(bool flag) { m_isDestructor = flag; }
    bool isMethod() ଅଚଳ { ଫେରନ୍ତୁ m_isMethod; }
    ଶୂନ୍ୟ isMethod(bool flag) { m_isMethod = flag; }
    bool isLoose() ଅଚଳ { ଫେରନ୍ତୁ m_isLoose; }
    ଶୂନ୍ୟ isLoose(bool flag) { m_isLoose = flag; }
    bool isProperMethod() ଅଚଳ { ଫେରନ୍ତୁ isMethod() && !isLoose(); }
    bool isVirtual() ଅଚଳ { ଫେରନ୍ତୁ m_isVirtual; }
    ଶୂନ୍ୟ isVirtual(bool flag) { m_isVirtual = flag; }
    bool needProcess() ଅଚଳ { ଫେରନ୍ତୁ m_needProcess; }
    ଶୂନ୍ୟ setNeedProcess() { m_needProcess = true; }
    bool entryPoint() ଅଚଳ { ଫେରନ୍ତୁ m_entryPoint; }
    ଶୂନ୍ୟ entryPoint(bool flag) { m_entryPoint = flag; }
    bool dpiPure() ଅଚଳ { ଫେରନ୍ତୁ m_dpiPure; }
    ଶୂନ୍ୟ dpiPure(bool flag) { m_dpiPure = flag; }
    bool dpiContext() ଅଚଳ { ଫେରନ୍ତୁ m_dpiContext; }
    ଶୂନ୍ୟ dpiContext(bool flag) { m_dpiContext = flag; }
    bool dpiExportDispatcher() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_dpiExportDispatcher; }
    ଶୂନ୍ୟ dpiExportDispatcher(bool flag) { m_dpiExportDispatcher = flag; }
    bool dpiExportImpl() ଅଚଳ { ଫେରନ୍ତୁ m_dpiExportImpl; }
    ଶୂନ୍ୟ dpiExportImpl(bool flag) { m_dpiExportImpl = flag; }
    bool dpiImportPrototype() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_dpiImportPrototype; }
    ଶୂନ୍ୟ dpiImportPrototype(bool flag) { m_dpiImportPrototype = flag; }
    bool dpiImportWrapper() ଅଚଳ { ଫେରନ୍ତୁ m_dpiImportWrapper; }
    ଶୂନ୍ୟ dpiImportWrapper(bool flag) { m_dpiImportWrapper = flag; }
    bool dpiCDeclOverride() ଅଚଳ { ଫେରନ୍ତୁ !m_dpiCDecl.empty(); }
    ଅଚଳ string& dpiCDecl() ଅଚଳ { ଫେରନ୍ତୁ m_dpiCDecl; }
    ଶୂନ୍ୟ dpiCDecl(ଅଚଳ string& cDecl) { m_dpiCDecl = cDecl; }
    bool isCoroutine() ଅଚଳ { ଫେରନ୍ତୁ m_rtnType == "VlCoroutine"; }
    ଶୂନ୍ୟ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ଅଚଳ { ଫେରନ୍ତୁ m_recursive; }
    ଶୂନ୍ୟ unlikely(bool flag) { m_unlikely = flag; }
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ m_unlikely; }  // Note virtual override
    ଶୂନ୍ୟ noLife(bool flag) { m_noLife = flag; }
    bool noLife() ଅଚଳ { ଫେରନ୍ତୁ m_noLife; }
    bool isCovergroupSample() ଅଚଳ { ଫେରନ୍ତୁ m_isCovergroupSample; }
    ଶୂନ୍ୟ isCovergroupSample(bool flag) { m_isCovergroupSample = flag; }
    bool voidSelf() ଅଚଳ { ଫେରନ୍ତୁ m_voidSelf; }
    ଶୂନ୍ୟ voidSelf(bool flag) { m_voidSelf = flag; }
    ଶୂନ୍ୟ cost(ପୂର୍ଣ୍ଣସଂଖ୍ୟା cost) { m_cost = cost; }
    // Special methods
    bool emptyBody() ଅଚଳ {
        ଫେରନ୍ତୁ !keepIfEmpty() && !argsp() && !varsp() && !stmtsp() && !scopeNamep() && !isVirtual()
               && !dpiImportPrototype();
    }
};
class AstCUse final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // C++ use of a class or #include; indicates need of forward declaration
    // Parents:  NODEMODULE
    ଅଚଳ string m_name;
    ଅଚଳ VUseType m_useType;  // What sort of use this is

ସର୍ବସାଧାରଣ:
    AstCUse(File_ଧାଡ଼ି* fl, VUseType useType, ଅଚଳ string& name)
        : ASTGEN_SUPER_CUse(fl)
        , m_name{name}
        , m_useType{useType} {}
    ASTGEN_MEMBERS_AstCUse;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    VUseType useType() ଅଚଳ { ଫେରନ୍ତୁ m_useType; }
};
class AstCell final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
    File_ଧାଡ଼ି* m_modNameFileline;  // Where module the cell instances token was
    string m_name;  // Cell name
    string m_origName;  // Original name before dot addition
    string m_modName;  // Module the cell instances
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_arrayIdx = -1;  // Row-major position in instance array, from the left (V3Param-V3Width)
    bool m_hasIfaceVar : 1;  // True if a Var has been created for this cell
    bool m_recursive : 1;  // Self-recursive module
    bool m_trace : 1;  // Trace this cell
ସର୍ବସାଧାରଣ:
    AstCell(File_ଧାଡ଼ି* fl, File_ଧାଡ଼ି* mfl, ଅଚଳ string& instName, ଅଚଳ string& modName,
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
    ଶୂନ୍ୟ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    // ACCESSORS
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Cell name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string origName() ଅଚଳ override { ଫେରନ୍ତୁ m_origName; }  // * = Original name
    ଶୂନ୍ୟ origName(ଅଚଳ string& name) { m_origName = name; }
    string modName() ଅଚଳ { ଫେରନ୍ତୁ m_modName; }  // * = Instance name
    ଶୂନ୍ୟ modName(ଅଚଳ string& name) { m_modName = name; }
    File_ଧାଡ଼ି* modNameFileline() ଅଚଳ { ଫେରନ୍ତୁ m_modNameFileline; }
    Ast_ଗଣ୍ଠି_Module* modp() ଅଚଳ { ଫେରନ୍ତୁ m_modp; }  // [AfterLink] = Pointer to module instantiated
    ଶୂନ୍ୟ modp(Ast_ଗଣ୍ଠି_Module* nodep) { m_modp = nodep; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା arrayIdx() ଅଚଳ { ଫେରନ୍ତୁ m_arrayIdx; }
    ଶୂନ୍ୟ arrayIdx(ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx) { m_arrayIdx = idx; }
    bool hasIfaceVar() ଅଚଳ { ଫେରନ୍ତୁ m_hasIfaceVar; }
    ଶୂନ୍ୟ hasIfaceVar(bool flag) { m_hasIfaceVar = flag; }
    ଶୂନ୍ୟ trace(bool flag) { m_trace = flag; }
    bool isTrace() ଅଚଳ { ଫେରନ୍ତୁ m_trace; }
    ଶୂନ୍ୟ recursive(bool flag) { m_recursive = flag; }
    bool recursive() ଅଚଳ { ଫେରନ୍ତୁ m_recursive; }
};
class AstCellInline final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A named block flattened by V3Begin, for name resolution in V3LinkDot.
    // Deleted by V3LinkDot::linkDotScope, after V3Scope made AstCellInlineScope for VPI.
    ଅଚଳ std::string m_name;  // Block path within the module, possibly {a}__DOT__{b}...
    ଅଚଳ std::string m_origModName;  // __BEGIN__ or __FORK__
ସର୍ବସାଧାରଣ:
    AstCellInline(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& name, ଅଚଳ std::string& origModName)
        : ASTGEN_SUPER_CellInline(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInline;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    // ACCESSORS
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    std::string origModName() ଅଚଳ { ଫେରନ୍ତୁ m_origModName; }
};
class AstCellInlineScope final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A named block or inlined cell flattened into a scope, kept so it can be
    // reported as a VPI scope. Only created with --vpi, by V3Scope for named
    // blocks, and by V3Inline for inlined cells.
    // Parents: Scope
    // Children: none
    std::string m_name;  // Flattened name relative to the containing scope
    ଅଚଳ std::string m_origModName;  // Original name of module, or __BEGIN__/__FORK__
ସର୍ବସାଧାରଣ:
    AstCellInlineScope(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& name, ଅଚଳ std::string& origModName)
        : ASTGEN_SUPER_CellInlineScope(fl)
        , m_name{name}
        , m_origModName{origModName} {}
    ASTGEN_MEMBERS_AstCellInlineScope;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    // ACCESSORS
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ std::string& name) override { m_name = name; }
    std::string origModName() ଅଚଳ { ଫେରନ୍ତୁ m_origModName; }
};
class AstCgOptionAssign final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A covergroup set of option
    // Parents: CLASS(covergroup) or cross
    ଅଚଳ VCoverOptionType m_optType;  // Option type
    ଅଚଳ string m_name;  // Original option name (for diagnostics on unknown options)
    ଅଚଳ bool m_typeOption;  // type_option vs option
    // @astgen op1 := valuep : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstCgOptionAssign(File_ଧାଡ଼ି* fl, bool typeOption, VCoverOptionType optType,
                      ଅଚଳ string& rawName, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_CgOptionAssign(fl)
        , m_optType{optType}
        , m_name{rawName}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCgOptionAssign;
    // ACCESSORS
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    VCoverOptionType optType() ଅଚଳ { ଫେରନ୍ତୁ m_optType; }
    bool typeOption() ଅଚଳ { ଫେରନ୍ତୁ m_typeOption; }
};
class AstClassExtends final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // class extends class name, or class implements class name
    // Children: List of AstParseRef for packages/classes
    // during early parse, then moves to dtype
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op2 := classOrPkgsp : Optional[AstNode]
    // @astgen op3 := argsp : List[AstArg]
    ଅଚଳ bool m_isImplements;  // class implements
    bool m_parameterized = false;  // has parameters in its statement

ସର୍ବସାଧାରଣ:
    AstClassExtends(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* classOrPkgsp, bool isImplements)
        : ASTGEN_SUPER_ClassExtends(fl)
        , m_isImplements{isImplements} {
        this->classOrPkgsp(classOrPkgsp);  // Only for parser
    }
    ASTGEN_MEMBERS_AstClassExtends;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ isImplements() ? "implements" : "extends"; }
    // Class being extended (after link and instantiation if needed)
    AstClass* classOrNullp() ଅଚଳ;
    AstClass* classp() ଅଚଳ;  // Like above, but throws error if nulll
    bool isImplements() ଅଚଳ { ଫେରନ୍ତୁ m_isImplements; }
    ଶୂନ୍ୟ parameterized(bool flag) { m_parameterized = flag; }
    bool parameterized() ଅଚଳ { ଫେରନ୍ତୁ m_parameterized; }
};
class AstClocking final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents:  MODULE
    // Children: SENTREE, CLOCKING ITEMs, VARs
    // @astgen op1 := sentreep : AstSenTree
    // @astgen op2 := itemsp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstVar]
    std::string m_name;  // Clocking block name
    bool m_isDefault;  // True if default clocking
    ଅଚଳ bool m_isGlobal;  // True if global clocking

ସର୍ବସାଧାରଣ:
    AstClocking(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& name, Ast_Sen_ବୃକ୍ଷ* sentreep, Ast_ଗଣ୍ଠି* itemsp,
                bool isDefault, bool isGlobal)
        : ASTGEN_SUPER_Clocking(fl)
        , m_name{name}
        , m_isDefault{isDefault}
        , m_isGlobal{isGlobal} {
        this->sentreep(sentreep);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstClocking;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool isDefault() ଅଚଳ { ଫେରନ୍ତୁ m_isDefault; }
    bool isGlobal() ଅଚଳ { ଫେରନ୍ତୁ m_isGlobal; }
    AstVar* ensureEventp(bool childDType = false);
    ଶୂନ୍ୟ makeDefault() { m_isDefault = true; }
};
class AstClockingItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents:  CLOCKING
    // Children: EXPRs, ASSIGNs, VARs
    // @astgen op1 := skewp : Optional[AstNodeExpr]
    // @astgen op2 := exprp : Optional[AstNodeExpr]
    // @astgen op3 := assignp : Optional[AstAssign]
    // @astgen op4 := varp : Optional[AstVar]
    // @astgen ptr := m_outputp : Optional[AstClockingItem]
    VDirection m_direction;

ସର୍ବସାଧାରଣ:
    AstClockingItem(File_ଧାଡ଼ି* fl, VDirection direction, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* skewp, Ast_ଗଣ୍ଠି* clockingDeclp)
        : ASTGEN_SUPER_ClockingItem(fl) {
        m_direction = direction;
        this->skewp(skewp);
        ଯଦି (AstAssign* ଅଚଳ clkAssignp = VN_CAST(clockingDeclp, Assign)) {
            this->assignp(clkAssignp);
        } ଅନ୍ୟଥା {
            exprp(VN_AS(clockingDeclp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି));
        }
    }
    ASTGEN_MEMBERS_AstClockingItem;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VDirection direction() ଅଚଳ { ଫେରନ୍ତୁ m_direction; }
    AstClockingItem* outputp() ଅଚଳ { ଫେରନ୍ତୁ m_outputp; }
    ଶୂନ୍ୟ outputp(AstClockingItem* outputp) { m_outputp = outputp; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
};
class AstConfig final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents: NETLIST
    // @astgen op1 := designp : List[AstConfigCell]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_libname;  // Config library, or ""
    std::string m_configname;  // Config name within library

ସର୍ବସାଧାରଣ:
    AstConfig(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& libname, ଅଚଳ std::string& cellname)
        : ASTGEN_SUPER_Config(fl)
        , m_libname{libname}
        , m_configname{cellname} {}
    ASTGEN_MEMBERS_AstConfig;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_libname + "." + m_configname; }
    std::string libname() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_libname; }
    std::string configname() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_configname; }
};
class AstConfigCell final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents: CONFIGRULE
    std::string m_libname;  // Cell library, or ""
    std::string m_cellname;  // Cell name within library

ସର୍ବସାଧାରଣ:
    AstConfigCell(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& libname, ଅଚଳ std::string& cellname)
        : ASTGEN_SUPER_ConfigCell(fl)
        , m_libname{libname}
        , m_cellname{cellname} {}
    ASTGEN_MEMBERS_AstConfigCell;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_libname + "." + m_cellname; }
    std::string libname() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_libname; }
    std::string cellname() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_cellname; }
};
class AstConfigRule final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents: CONFIG
    // @astgen op1 := cellp : Optional[AstNode]  // Cells to apply to, or nullptr=default
    // @astgen op2 := usep : List[AstNode]  // Use or design to apply
    ଅଚଳ bool m_isCell;  // Declared as "cell" versus "instance"

ସର୍ବସାଧାରଣ:
    AstConfigRule(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* cellp, Ast_ଗଣ୍ଠି* usep, bool isCell)
        : ASTGEN_SUPER_ConfigRule(fl)
        , m_isCell{isCell} {
        this->cellp(cellp);
        addUsep(usep);
    }
    ASTGEN_MEMBERS_AstConfigRule;
    bool isCell() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_isCell; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstConfigUse final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents: CONFIGRULE
    // @astgen op1 := paramsp : List[AstPin]
    std::string m_libname;  // Use library, or ""
    std::string m_cellname;  // Use name within library
    ଅଚଳ bool m_isConfig;  // ":config"; Config, not module/primitive name

ସର୍ବସାଧାରଣ:
    AstConfigUse(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& libname, ଅଚଳ std::string& cellname,
                 AstPin* paramsp, bool isConfig)
        : ASTGEN_SUPER_ConfigUse(fl)
        , m_libname{libname}
        , m_cellname{cellname}
        , m_isConfig{isConfig} {
        addParamsp(paramsp);
    }
    ASTGEN_MEMBERS_AstConfigUse;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_libname + "." + m_cellname; }
    std::string libname() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_libname; }
    std::string cellname() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_cellname; }
    bool isConfig() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_isConfig; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstConstraint final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
ସର୍ବସାଧାରଣ:
    AstConstraint(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* itemsp)
        : ASTGEN_SUPER_Constraint(fl)
        , m_name{name} {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstConstraint;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Scope name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }  // * = Scope name
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ baseOverride(ଅଚଳ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ଅଚଳ { ଫେରନ୍ତୁ m_baseOverride; }
    bool isExternDef() ଅଚଳ { ଫେରନ୍ତୁ m_isExternDef; }
    ଶୂନ୍ୟ isExternDef(bool flag) { m_isExternDef = flag; }
    ଶୂନ୍ୟ isExternExplicit(bool flag) { m_isExternExplicit = flag; }
    bool isExternExplicit() ଅଚଳ { ଫେରନ୍ତୁ m_isExternExplicit; }
    ଶୂନ୍ୟ isExternProto(bool flag) { m_isExternProto = flag; }
    bool isExternProto() ଅଚଳ { ଫେରନ୍ତୁ m_isExternProto; }
    ଶୂନ୍ୟ isKwdPure(bool flag) { m_isKwdPure = flag; }
    bool isKwdPure() ଅଚଳ { ଫେରନ୍ତୁ m_isKwdPure; }
    ଶୂନ୍ୟ isStatic(bool flag) { m_isStatic = flag; }
    bool isStatic() ଅଚଳ { ଫେରନ୍ତୁ m_isStatic; }
};
class AstCoverBin final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Captures data for a coverpoint 'bins' declaration
    // @astgen op1 := rangesp : List[AstNode]
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    // @astgen op3 := arraySizep : Optional[AstNodeExpr]
    // @astgen op4 := transp : List[AstCoverTransSet]
    ଅଚଳ string m_name;  // Base name of the bin
    ଅଚଳ VCoverBinsType m_binsType;  // Bin type (eg AUTO, IGNORE, ILLEGAL)
    bool m_isArray = false;  // Bin is an array: of a bin per value or transition, or of
                             // arraySizep() bins
    bool m_isWildcard = false;  // Bin uses wildcard matching (independent of ignore/illegal)

ସର୍ବସାଧାରଣ:
    AstCoverBin(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* rangesp, bool isIgnore, bool isIllegal,
                bool isWildcard = false, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* iffp = nullptr)
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
    AstCoverBin(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* arraySizep,
                VCoverBinsType type = VCoverBinsType::BINS_AUTO)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{true} {
        this->arraySizep(arraySizep);
    }
    // Constructor for default bins (catch-all)
    AstCoverBin(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, VCoverBinsType type)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type} {}
    // Constructor for transition bins
    AstCoverBin(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstCoverTransSet* transp,
                VCoverBinsType type = VCoverBinsType::BINS_TRANSITION, bool isArrayBin = false)
        : ASTGEN_SUPER_CoverBin(fl)
        , m_name{name}
        , m_binsType{type}
        , m_isArray{isArrayBin} {
        UASSERT(transp, "AstCoverBin transition constructor requires non-null transp");
        addTransp(transp);
    }
    ASTGEN_MEMBERS_AstCoverBin;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    VCoverBinsType binsType() ଅଚଳ { ଫେରନ୍ତୁ m_binsType; }
    bool isWildcard() ଅଚଳ { ଫେରନ୍ତୁ m_isWildcard; }
    bool isArray() ଅଚଳ { ଫେରନ୍ତୁ m_isArray; }
    ଶୂନ୍ୟ isArray(bool flag) { m_isArray = flag; }
};
class AstCoverBinsof final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A binsof selection of a coverpoint or one of its named bins
    // @astgen op1 := pointp : AstCoverpointRef
    // @astgen op2 := rangesp : List[AstNode]  // Optional intersect value ranges
    string m_name;  // Selected bin name, or empty for all bins of the coverpoint
    ଅଚଳ bool m_isNegated;  // Complement the selection within the cross product

ସର୍ବସାଧାରଣ:
    AstCoverBinsof(File_ଧାଡ଼ି* fl, AstCoverpointRef* pointp, bool isNegated = false,
                   Ast_ଗଣ୍ଠି* rangesp = nullptr)
        : ASTGEN_SUPER_CoverBinsof(fl)
        , m_isNegated{isNegated} {
        this->pointp(pointp);
        addRangesp(rangesp);
    }
    ASTGEN_MEMBERS_AstCoverBinsof;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    bool isNegated() ଅଚଳ { ଫେରନ୍ତୁ m_isNegated; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_START
        ଅଚଳ AstCoverBinsof* ଅଚଳ asamep = VN_DBG_AS(samep, CoverBinsof);
        ଫେରନ୍ତୁ m_name == asamep->m_name && m_isNegated == asamep->m_isNegated;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossBin final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A named cross bin and its selection expression
    // @astgen op1 := selectp : Optional[AstNode]  // Null for unsupported selections
    // @astgen op2 := iffp : Optional[AstNodeExpr]
    ଅଚଳ string m_name;  // Declared cross bin name
    // dist-ast-dump-suppress  // Bin kind is shown by verilogKwd() in emitted Verilog.
    ଅଚଳ VCoverBinsType m_binsType;  // Normal, ignore, or illegal bin

ସର୍ବସାଧାରଣ:
    AstCoverCrossBin(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* selectp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* iffp,
                     VCoverBinsType binsType = VCoverBinsType::BINS_USER)
        : ASTGEN_SUPER_CoverCrossBin(fl)
        , m_name{name}
        , m_binsType{binsType} {
        this->selectp(selectp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCrossBin;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    string verilogKwd() ଅଚଳ override;
    VCoverBinsType binsType() ଅଚଳ { ଫେରନ୍ତୁ m_binsType; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_START
        ଅଚଳ AstCoverCrossBin* ଅଚଳ asamep = VN_DBG_AS(samep, CoverCrossBin);
        ଫେରନ୍ତୁ m_name == asamep->m_name && m_binsType.m_e == asamep->m_binsType.m_e;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // The complete bin-tuple space of the enclosing cross
    ଅଚଳ string m_name;  // Referenced cross name

ସର୍ବସାଧାରଣ:
    AstCoverCrossRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_CoverCrossRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstCoverCrossRef;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_START
        ଫେରନ୍ତୁ m_name == VN_DBG_AS(samep, CoverCrossRef)->m_name;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossSelect final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Intersection or union of two cross-bin selections
    // @astgen op1 := lhsp : Optional[AstNode]  // Null for an unsupported selection
    // @astgen op2 := rhsp : Optional[AstNode]  // Null for an unsupported selection
    ଅଚଳ bool m_isOr;  // Union (||), rather than intersection (&&)

ସର୍ବସାଧାରଣ:
    AstCoverCrossSelect(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* lhsp, Ast_ଗଣ୍ଠି* rhsp, bool isOr)
        : ASTGEN_SUPER_CoverCrossSelect(fl)
        , m_isOr{isOr} {
        this->lhsp(lhsp);
        this->rhsp(rhsp);
    }
    ASTGEN_MEMBERS_AstCoverCrossSelect;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool isOr() ଅଚଳ { ଫେରନ୍ତୁ m_isOr; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ isOr() ? "||" : "&&"; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_START
        ଫେରନ୍ତୁ m_isOr == VN_DBG_AS(samep, CoverCrossSelect)->m_isOr;
    }  // LCOV_EXCL_STOP
};
class AstCoverCrossWith final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // The bin tuples of a cross-bin selection with enough value tuples for which a 'with' filter
    // is true: one, or as many as a 'matches' clause gives (IEEE 1800-2023 19.6.1.2)
    // @astgen op1 := selectp : Optional[AstNode]  // Selection filtered; null if unsupported
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := varsp : List[AstVar]  // Candidate value of each cross item, from V3LinkParse
    // @astgen op4 := countp : Optional[AstNodeExpr]  // Of 'matches', AstUnbounded for '$'
ସର୍ବସାଧାରଣ:
    AstCoverCrossWith(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* selectp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filterp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* countp)
        : ASTGEN_SUPER_CoverCrossWith(fl) {
        this->selectp(selectp);
        this->filterp(filterp);
        this->countp(countp);
    }
    ASTGEN_MEMBERS_AstCoverCrossWith;
};
class AstCoverOption final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Coverage-option assignment
    // @astgen op1 := valuep : AstNodeExpr
    ଅଚଳ VCoverOptionType m_optType;  // Option being assigned
    ଅଚଳ bool m_typeOption;  // type_option (vs option)

ସର୍ବସାଧାରଣ:
    AstCoverOption(File_ଧାଡ଼ି* fl, bool typeOption, VCoverOptionType optType, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_CoverOption(fl)
        , m_optType{optType}
        , m_typeOption{typeOption} {
        this->valuep(valuep);
    }
    ASTGEN_MEMBERS_AstCoverOption;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VCoverOptionType optType() ଅଚଳ { ଫେରନ୍ତୁ m_optType; }
    bool typeOption() ଅଚଳ { ଫେରନ୍ତୁ m_typeOption; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {  // LCOV_EXCL_START
        ଅଚଳ AstCoverOption* ଅଚଳ asamep = VN_DBG_AS(samep, CoverOption);
        ଫେରନ୍ତୁ m_optType.m_e == asamep->m_optType.m_e && m_typeOption == asamep->m_typeOption;
    }  // LCOV_EXCL_STOP
};
class AstCoverToggle final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Toggle analysis of given signal
    // Parents:  MODULE
    // @astgen op1 := incp : AstCoverInc
    // @astgen op2 := origp : AstNodeExpr
    // @astgen op3 := changep : AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstCoverToggle(File_ଧାଡ଼ି* fl, AstCoverInc* incp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* origp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* changep)
        : ASTGEN_SUPER_CoverToggle(fl) {
        this->incp(incp);
        this->origp(origp);
        this->changep(changep);
    }
    ASTGEN_MEMBERS_AstCoverToggle;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ 3 + INSTR_ଗଣନା_BRANCH + INSTR_ଗଣନା_LD; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isOutputter() override {
        ଫେରନ୍ତୁ false;  // Though the AstCoverInc under this is an outputter
    }
};
class AstCoverTransItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Represents a single transition item: value or value[*N] or value[->N] or value[=N]
    // @astgen op1 := valuesp : List[AstNode]
    // @astgen op2 := repMinp : Optional[AstNodeExpr]
    // @astgen op3 := repMaxp : Optional[AstNodeExpr]
    ଅଚଳ VTransRepType m_repType;

ସର୍ବସାଧାରଣ:
    AstCoverTransItem(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* valuesp, VTransRepType repType = VTransRepType::NONE)
        : ASTGEN_SUPER_CoverTransItem(fl)
        , m_repType{repType} {
        addValuesp(valuesp);
    }
    ASTGEN_MEMBERS_AstCoverTransItem;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstCoverTransSet final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Represents a transition set: value1 => value2 => value3
    // @astgen op1 := itemsp : List[AstCoverTransItem]
ସର୍ବସାଧାରଣ:
    AstCoverTransSet(File_ଧାଡ଼ି* fl, AstCoverTransItem* itemsp)
        : ASTGEN_SUPER_CoverTransSet(fl) {
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstCoverTransSet;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstCoverWith final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // The values of a coverpoint bin for which a 'with' filter is true (IEEE 1800-2023
    // 19.5.1.1), the only range list element of its AstCoverBin
    // @astgen op1 := subp : List[AstNode]  // Range list, or AstCoverpointRef for all values
    // @astgen op2 := filterp : AstNodeExpr
    // @astgen op3 := itemp : Optional[AstVar]  // Candidate value 'item', from V3LinkParse
ସର୍ବସାଧାରଣ:
    AstCoverWith(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* subp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filterp)
        : ASTGEN_SUPER_CoverWith(fl) {
        addSubp(subp);
        this->filterp(filterp);
    }
    ASTGEN_MEMBERS_AstCoverWith;
};
class AstCovergroup final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Represents a covergroup declaration. V3LinkParse transforms this
    // into an AstClass with isCovergroup==true and attaches the clocking
    // event as a new AstCovergroup sentinel in class membersp.
    // @astgen op1 := argsp : List[AstVar]
    // @astgen op2 := membersp : List[AstNode]
    // @astgen op3 := eventp : Optional[AstSenTree]
    // @astgen op4 := sampleArgsp : List[AstVar]
    string m_name;  // covergroup name

ସର୍ବସାଧାରଣ:
    AstCovergroup(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstVar* argsp, AstVar* sampleArgsp,
                  Ast_ଗଣ୍ଠି* membersp, Ast_Sen_ବୃକ୍ଷ* eventp)
        : ASTGEN_SUPER_Covergroup(fl)
        , m_name{name} {
        addArgsp(argsp);
        addSampleArgsp(sampleArgsp);
        addMembersp(membersp);
        this->eventp(eventp);
    }
    ASTGEN_MEMBERS_AstCovergroup;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    bool maybePointedTo() ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstCoverpointRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Reference to a coverpoint used in a cross
    // @astgen op1 := exprp : Optional[AstNodeExpr]  // Non-standard: hierarchical/dotted
    //                                               // reference (implicit coverpoint), e.g.
    //                                               // 'cross a.b'; nullptr for a plain coverpoint
    //                                               // name.  An AstDot at parse time, resolved
    //                                               // by the time V3Covergroup runs.
    ଅଚଳ string m_name;  // coverpoint name; empty when exprp() carries a hierarchical reference

ସର୍ବସାଧାରଣ:
    AstCoverpointRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_CoverpointRef(fl)
        , m_name{name} {}
    AstCoverpointRef(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp)
        : ASTGEN_SUPER_CoverpointRef(fl) {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstCoverpointRef;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
};
class AstDefParam final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A defparam assignment
    // Parents: MODULE
    // @astgen op1 := rhsp : AstNodeExpr
    // @astgen op2 := pathp : AstNodeExpr
    string m_name;  // Name of variable getting set
ସର୍ବସାଧାରଣ:
    AstDefParam(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* pathp, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rhsp)
        : ASTGEN_SUPER_DefParam(fl)
        , m_name{name} {
        this->rhsp(rhsp);
        this->pathp(pathp);
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Scope name
    ASTGEN_MEMBERS_AstDefParam;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstDefaultClocking final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    std::string m_name;  // Clocking block name

ସର୍ବସାଧାରଣ:
    AstDefaultClocking(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& name)
        : ASTGEN_SUPER_DefaultClocking(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstDefaultClocking;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
};
class AstDefaultDisable final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // @astgen op1 := condp : AstNodeExpr

ସର୍ବସାଧାରଣ:
    AstDefaultDisable(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp)
        : ASTGEN_SUPER_DefaultDisable(fl) {
        this->condp(condp);
    }
    ASTGEN_MEMBERS_AstDefaultDisable;
};
class AstDpiExport final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // We could put an AstNodeFTaskRef instead of the verilog function name,
    // however we're not *calling* it, so that seems somehow wrong.
    // (Probably AstNodeFTaskRef should be renamed AstNodeFTaskCall and have-a AstNodeFTaskRef)
    string m_name;  // Name of function
    string m_cname;  // Name of function on c side
ସର୍ବସାଧାରଣ:
    AstDpiExport(File_ଧାଡ଼ି* fl, ଅଚଳ string& vname, ଅଚଳ string& cname)
        : ASTGEN_SUPER_DpiExport(fl)
        , m_name{vname}
        , m_cname{cname} {}
    ASTGEN_MEMBERS_AstDpiExport;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string cname() ଅଚଳ { ଫେରନ୍ତୁ m_cname; }
    ଶୂନ୍ୟ cname(ଅଚଳ string& cname) { m_cname = cname; }
};
class AstElabDisplay final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents: stmtlist
    // @astgen op1 := fmtp : List[AstSFormatF]
    VDisplayType m_displayType;

ସର୍ବସାଧାରଣ:
    ଧାଡ଼ିଭିତରେ AstElabDisplay(File_ଧାଡ଼ି* fl, VDisplayType dispType, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprsp);
    ASTGEN_MEMBERS_AstElabDisplay;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!fmtp());
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "$"s + string{displayType().ascii()}; }
    bool isGateOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }  // SPECIAL: $display has 'visual' ordering
    bool isOutputter() override { ଫେରନ୍ତୁ true; }  // SPECIAL: $display makes output
    bool isUnlikely() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ displayType() == VN_DBG_AS(samep, ElabDisplay)->displayType();
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା instr_ଗଣନା() ଅଚଳ override { ଫେରନ୍ତୁ INSTR_ଗଣନା_PLI; }
    VDisplayType displayType() ଅଚଳ { ଫେରନ୍ତୁ m_displayType; }
    ଶୂନ୍ୟ displayType(VDisplayType type) { m_displayType = type; }
};
class AstEmpty final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Represents something missing, e.g. a missing argument in FOREACH
ସର୍ବସାଧାରଣ:
    explicit AstEmpty(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_Empty(fl) {}
    ASTGEN_MEMBERS_AstEmpty;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstGenCaseItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Single item of an AstGenCase
    // @astgen op1 := condsp : List[AstNodeExpr]
    // @astgen op2 := itemsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstGenCaseItem(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condsp, Ast_ଗଣ୍ଠି* itemsp)
        : ASTGEN_SUPER_GenCaseItem(fl) {
        addCondsp(condsp);
        addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCaseItem;
    bool isDefault() ଅଚଳ { ଫେରନ୍ତୁ condsp() == nullptr; }
};
class AstImplicit final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Create implicit wires and do nothing else, for gates that are ignored
    // Parents: MODULE
    // @astgen op1 := exprsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstImplicit(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* exprsp)
        : ASTGEN_SUPER_Implicit(fl) {
        addExprsp(exprsp);
    }
    ASTGEN_MEMBERS_AstImplicit;
};
class AstIntfRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // An interface reference
    string m_name;  // Hierarchical path of the reference
    string m_parentName;  // Hierarchical path of the declaring module, before inlining
    string m_baseName;  // Final component of m_name, i.e. the reference port name
    string m_modportName;  // "" = no modport, else name of the modport referenced
ସର୍ବସାଧାରଣ:
    AstIntfRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& parentName, ଅଚଳ string& baseName,
               ଅଚଳ string& modportName)
        : ASTGEN_SUPER_IntfRef(fl)
        , m_name{name}
        , m_parentName{parentName}
        , m_baseName{baseName}
        , m_modportName{modportName} {}
    ASTGEN_MEMBERS_AstIntfRef;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    string parentName() ଅଚଳ { ଫେରନ୍ତୁ m_parentName; }
    string baseName() ଅଚଳ { ଫେରନ୍ତୁ m_baseName; }
    string modportName() ଅଚଳ { ଫେରନ୍ତୁ m_modportName; }
};
class AstLibrary final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents: NETLIST
    // Library declaration from lib.map file
    // @astgen op1 := filesp : List[AstNodeExpr]
    // @astgen op2 := incdirsp : List[AstNodeExpr]
    std::string m_name;  // Library name

ସର୍ବସାଧାରଣ:
    AstLibrary(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* filesp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* incdirsp)
        : ASTGEN_SUPER_Library(fl)
        , m_name{name} {
        addFilesp(filesp);
        addIncdirsp(incdirsp);
    }
    ASTGEN_MEMBERS_AstLibrary;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
};
class AstModport final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A modport in an interface
    // @astgen op1 := varsp : List[AstNode]
    string m_name;  // Name of the modport
ସର୍ବସାଧାରଣ:
    AstModport(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* varsp)
        : ASTGEN_SUPER_Modport(fl)
        , m_name{name} {
        addVarsp(varsp);
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "modport"; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ASTGEN_MEMBERS_AstModport;
};
class AstModportClockingRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A clocking block referenced under a modport
    // The storage for the clocking block itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_clockingp : Optional[AstClocking]  // Link to the actual clocking block
    string m_name;  // Name of the clocking block referenced
ସର୍ବସାଧାରଣ:
    AstModportClockingRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_ModportClockingRef(fl)
        , m_name{name} {}
    ASTGEN_MEMBERS_AstModportClockingRef;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    AstClocking* clockingp() ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ m_clockingp;
    }  // [After Link] Pointer to clocking block
    ଶୂନ୍ୟ clockingp(AstClocking* clockingp) { m_clockingp = clockingp; }
};
class AstModportFTaskRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // An import/export referenced under a modport
    // The storage for the function itself is inside the
    // interface/instantiator, thus this is a reference
    // PARENT: AstModport
    //
    // @astgen ptr := m_ftaskp : Optional[AstNodeFTask]  // Link to the function
    string m_name;  // Name of the variable referenced
    bool m_export;  // Type of the function (import/export)
ସର୍ବସାଧାରଣ:
    AstModportFTaskRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, bool isExport)
        : ASTGEN_SUPER_ModportFTaskRef(fl)
        , m_name{name}
        , m_export{isExport} {}
    ASTGEN_MEMBERS_AstModportFTaskRef;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool isImport() ଅଚଳ { ଫେରନ୍ତୁ !m_export; }
    bool isExport() ଅଚଳ { ଫେରନ୍ତୁ m_export; }
    Ast_ଗଣ୍ଠି_FTask* ftaskp() ଅଚଳ { ଫେରନ୍ତୁ m_ftaskp; }  // [After Link] Pointer to variable
    ଶୂନ୍ୟ ftaskp(Ast_ଗଣ୍ଠି_FTask* ftaskp) { m_ftaskp = ftaskp; }
};
class AstModportVarRef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A input/output/etc variable referenced under a modport
    // The storage for the variable itself is inside the interface, thus this is a reference
    // PARENT: AstModport
    // @astgen op1 := exprp : Optional[AstNodeExpr]
    //
    // @astgen ptr := m_varp : Optional[AstVar]  // Link to the actual Var
    string m_name;  // Name of the variable referenced
    ଅଚଳ VDirection m_direction;  // Direction of the variable (in/out)
ସର୍ବସାଧାରଣ:
    AstModportVarRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {}
    AstModportVarRef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                     VDirection::en direction)
        : ASTGEN_SUPER_ModportVarRef(fl)
        , m_name{name}
        , m_direction{direction} {
        this->exprp(exprp);
    };
    ASTGEN_MEMBERS_AstModportVarRef;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    VDirection direction() ଅଚଳ { ଫେରନ୍ତୁ m_direction; }
    AstVar* varp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_varp; }  // [After Link] Pointer to variable
    ଶୂନ୍ୟ varp(AstVar* varp) { m_varp = varp; }
};
class AstNetlist final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
    std::string m_resolved_ଶୀର୍ଷ_Module_Name;  // Selected design top before wrapping under $root
    bool m_timescaleSpecified = false;  // Input HDL specified timescale
    uint32_t m_nTraceCodes = 0;  // Number of trace codes used by design
    // V3Param-deferred params awaiting V3LinkDot::linkDotParamed scope-resolution.
    std::set<AstVar*> m_deferredParamVarps;
    // Sparse metadata for constants produced from named parameters/localparams. Keep this off
    // AstConst itself, as AstConst is a very common node and only a small fraction carry this
    // name.
    std::unordered_map<ଅଚଳ AstConst*, string> m_constOrigParamNames;
    // Module each node is in, only good while the tree holds still
    std::unordered_map<ଅଚଳ Ast_ଗଣ୍ଠି*, ଅଚଳ Ast_ଗଣ୍ଠି_Module*> m_containingModules;
    // The model's evaluation entry point functions
    std::array<AstCFunc*, VEval::_ENUM_END> m_evalFuncps{};
    // The trigger dump function of each region if exists, otherwise nullptr
    std::array<AstCFunc*, VEval::_ENUM_END> m_dumpTriggersFuncps{};

ସର୍ବସାଧାରଣ:
    AstNetlist();
    ASTGEN_MEMBERS_AstNetlist;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override;
    ଶୂନ୍ୟ ଠେଲ_Deferred_Param_Varp(AstVar* varp) { m_deferredParamVarps.ଭର୍ତ୍ତି(varp); }
    ଅଚଳ std::set<AstVar*>& deferredParamVarps() ଅଚଳ { ଫେରନ୍ତୁ m_deferredParamVarps; }
    ଶୂନ୍ୟ clearDeferredParamVarps() { m_deferredParamVarps.clear(); }
    ଶୂନ୍ୟ deleteContents();
    ଶୂନ୍ୟ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ "$root"; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    Ast_ଗଣ୍ଠି_Module* ଶୀର୍ଷ_Modulep() ଅଚଳ VL_MT_STABLE {  // Top module in hierarchy
        ଫେରନ୍ତୁ modulesp();  // First one in the list, for now
    }
    AstTypeTable* typeTablep() { ଫେରନ୍ତୁ m_typeTablep; }
    string astConstOrigParamName(ଅଚଳ AstConst* nodep) ଅଚଳ;
    ଶୂନ୍ୟ astConstOrigParamName(ଅଚଳ AstConst* nodep, ଅଚଳ string& name);
    ଶୂନ୍ୟ astConstOrigParamNameErase(ଅଚଳ AstConst* nodep);
    // Find the module a node is in, remembering what it passed on the way.
    ଅଚଳ Ast_ଗଣ୍ଠି_Module* containingModule(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);
    // Forget remembered modules, as the tree has moved.
    ଶୂନ୍ୟ clearContainingModules() { m_containingModules.clear(); }
    AstPackage* constPoolPkgp() ଅଚଳ { ଫେରନ୍ତୁ m_constPoolPkgp; }
    AstPackage* dollarUnitPkgp() ଅଚଳ { ଫେରନ୍ତୁ m_dollarUnitPkgp; }
    ଶୂନ୍ୟ dollarUnitPkgp(AstPackage* ଅଚଳ packagep) { m_dollarUnitPkgp = packagep; }
    // Returns the entry point eval function, lazy constructed on first call
    AstCFunc* evalFuncp(VEval eval);
    // Returns the trigger dump function of the given eval function, lazy constructed
    AstCFunc* dumpTriggersFuncp(VEval eval);
    AstVarScope* dpiExportTriggerp() ଅଚଳ { ଫେରନ୍ତୁ m_dpiExportTriggerp; }
    ଶୂନ୍ୟ dpiExportTriggerp(AstVarScope* varScopep) { m_dpiExportTriggerp = varScopep; }
    AstVar* delaySchedulerp() ଅଚଳ { ଫେରନ୍ତୁ m_delaySchedulerp; }
    ଶୂନ୍ୟ delaySchedulerp(AstVar* ଅଚଳ varScopep) { m_delaySchedulerp = varScopep; }
    AstVarScope* nbaEventp() ଅଚଳ { ଫେରନ୍ତୁ m_nbaEventp; }
    ଶୂନ୍ୟ nbaEventp(AstVarScope* ଅଚଳ varScopep) { m_nbaEventp = varScopep; }
    AstVarScope* nbaEventTriggerp() ଅଚଳ { ଫେରନ୍ତୁ m_nbaEventTriggerp; }
    ଶୂନ୍ୟ nbaEventTriggerp(AstVarScope* ଅଚଳ varScopep) { m_nbaEventTriggerp = varScopep; }
    ଶୂନ୍ୟ stdPackagep(AstPackage* ଅଚଳ packagep) { m_stdPackagep = packagep; }
    AstPackage* stdPackagep() ଅଚଳ { ଫେରନ୍ତୁ m_stdPackagep; }
    ଶୂନ୍ୟ stdPackageProcessp(AstClass* ଅଚଳ classp) { m_stdPackageProcessp = classp; }
    AstClass* stdPackageProcessp() ଅଚଳ { ଫେରନ୍ତୁ m_stdPackageProcessp; }
    AstFuncRef* stdPackageProcessSelfp(File_ଧାଡ଼ି*) ଅଚଳ;
    Ast_ଶୀର୍ଷ_Scope* ଶୀର୍ଷ_Scopep() ଅଚଳ { ଫେରନ୍ତୁ m_ଶୀର୍ଷ_Scopep; }
    ଶୂନ୍ୟ create_ଶୀର୍ଷ_Scope(AstScope* scopep);
    VTimescale timeunit() ଅଚଳ { ଫେରନ୍ତୁ m_timeunit; }
    ଶୂନ୍ୟ timeunit(ଅଚଳ VTimescale& ମୂଲ୍ୟ) { m_timeunit = ମୂଲ୍ୟ; }
    VTimescale timeprecision() ଅଚଳ { ଫେରନ୍ତୁ m_timeprecision; }
    ଶୂନ୍ୟ timeInit() {
        m_timeunit = v3Global.opt.timeDefaultUnit();
        m_timeprecision = v3Global.opt.timeDefaultPrec();
    }
    ଶୂନ୍ୟ timeprecisionMerge(File_ଧାଡ଼ି*, ଅଚଳ VTimescale& ମୂଲ୍ୟ);
    ଶୂନ୍ୟ timescaleSpecified(bool specified) { m_timescaleSpecified = specified; }
    bool timescaleSpecified() ଅଚଳ { ଫେରନ୍ତୁ m_timescaleSpecified; }
    ଅଚଳ std::string& resolved_ଶୀର୍ଷ_Module_Name() ଅଚଳ { ଫେରନ୍ତୁ m_resolved_ଶୀର୍ଷ_Module_Name; }
    ଶୂନ୍ୟ resolved_ଶୀର୍ଷ_Module_Name(ଅଚଳ std::string& ମୂଲ୍ୟ) { m_resolved_ଶୀର୍ଷ_Module_Name = ମୂଲ୍ୟ; }
    uint32_t nTraceCodes() ଅଚଳ { ଫେରନ୍ତୁ m_nTraceCodes; }
    ଶୂନ୍ୟ nTraceCodes(uint32_t ମୂଲ୍ୟ) { m_nTraceCodes = ମୂଲ୍ୟ; }
    AstVarScope* stlFirstIterationp();
    ଶୂନ୍ୟ clearStlFirstIterationp() { m_stlFirstIterationp = nullptr; }
    ଅଚଳ std::string trace_Lib_ଶୀର୍ଷ_Name() ଅଚଳ {
        ଅଚଳ std::string& name = resolved_ଶୀର୍ଷ_Module_Name();
        ଫେରନ୍ତୁ prettyName(name.empty() ? v3Global.rootp()->ଶୀର୍ଷ_Modulep()->name() : name);
    }

    // Record statistics for eval functions
    ଶୂନ୍ୟ addEvalStats(ଅଚଳ std::string& phase);
};
class AstPackageExport final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A package export declaration
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ସର୍ବସାଧାରଣ:
    AstPackageExport(File_ଧାଡ଼ି* fl, ଅଚଳ string& pkgName, ଅଚଳ string& name)
        : ASTGEN_SUPER_PackageExport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageExport;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    string pkgName() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_pkgName; }
    string prettyPkgNameQ() ଅଚଳ { ଫେରନ୍ତୁ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ଅଚଳ { ଫେରନ୍ତୁ m_packagep; }
    ଶୂନ୍ୟ packagep(AstPackage* nodep) { m_packagep = nodep; }
};
class AstPackageExportStarStar final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A package export *::* declaration
ସର୍ବସାଧାରଣ:
    // cppcheck-suppress noExplicitConstructor
    AstPackageExportStarStar(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_PackageExportStarStar(fl) {}
    ASTGEN_MEMBERS_AstPackageExportStarStar;
};
class AstPackageImport final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A package import declaration
    // @astgen op1 := resolvedClassp : Optional[AstClassOrPackageRef]
    //
    // @astgen ptr := m_packagep : Optional[AstPackage]  // Package hierarchy
    string m_name;  // What imported e.g. "*"
    string m_pkgName;  // Module the cell instances

ସର୍ବସାଧାରଣ:
    AstPackageImport(File_ଧାଡ଼ି* fl, AstPackage* packagep, ଅଚଳ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_packagep{packagep} {
        pkgNameFrom();
    }
    AstPackageImport(File_ଧାଡ଼ି* fl, ଅଚଳ string& pkgName, ଅଚଳ string& name)
        : ASTGEN_SUPER_PackageImport(fl)
        , m_name{name}
        , m_pkgName{pkgName}
        , m_packagep{nullptr} {}
    ASTGEN_MEMBERS_AstPackageImport;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    string pkgName() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_pkgName; }
    string prettyPkgNameQ() ଅଚଳ { ଫେରନ୍ତୁ "'" + prettyName(pkgName()) + "'"; }
    AstPackage* packagep() ଅଚଳ { ଫେରନ୍ତୁ m_packagep; }
    ଶୂନ୍ୟ packagep(AstPackage* nodep) { m_packagep = nodep; }

ବ୍ୟକ୍ତିଗତ:
    ଶୂନ୍ୟ pkgNameFrom();
};
class AstPin final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A port or parameter assignment on an instantiation
    // @astgen op1 := exprp : Optional[AstNode<AstNodeExpr|AstNodeDType>]  // nullptr=unconnected
    //
    // @astgen ptr := m_modVarp : Optional[AstVar]  // Input/output connects to on submodule
    // @astgen ptr := m_modPTypep : Optional[AstParamTypeDType]  // Param type connects to on sub
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_pin_ଅଙ୍କ;  // Pin number
    string m_name;  // Pin name, or "" for number based interconnect
    string m_paramPath;  // Original defparam cell path, if this pin came from a defparam
    bool m_param = false;  // Pin connects to parameter
    bool m_svDotName = false;  // Pin is SystemVerilog .name'ed
    bool m_svImplicit = false;  // Pin is SystemVerilog .name'ed, allow implicit
ସର୍ବସାଧାରଣ:
    AstPin(File_ଧାଡ଼ି* fl, ପୂର୍ଣ୍ଣସଂଖ୍ୟା pin_ଅଙ୍କ, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* exprp)
        : ASTGEN_SUPER_Pin(fl)
        , m_pin_ଅଙ୍କ{pin_ଅଙ୍କ}
        , m_name{name} {
        this->exprp(exprp);
    }
    ASTGEN_MEMBERS_AstPin;
    ଶୂନ୍ୟ cloneRelink() override {}  // TODO V3Param shouldn't require avoiding cloneRelinkGen
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Pin name, ""=go by number
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string prettyOperatorName() ଅଚଳ override;
    bool dotStar() ଅଚଳ { ଫେରନ୍ତୁ name() == ".*"; }  // Fake name for .* connections until linked
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା pin_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_pin_ଅଙ୍କ; }
    AstVar* modVarp() ଅଚଳ { ଫେରନ୍ତୁ m_modVarp; }  // [After Link] Pointer to variable
    ଶୂନ୍ୟ modVarp(AstVar* nodep) { m_modVarp = nodep; }
    // [After Link] Pointer to variable
    AstParamTypeDType* modPTypep() ଅଚଳ { ଫେରନ୍ତୁ m_modPTypep; }
    ଶୂନ୍ୟ modPTypep(AstParamTypeDType* nodep) { m_modPTypep = nodep; }
    bool param() ଅଚଳ { ଫେରନ୍ତୁ m_param; }
    ଶୂନ୍ୟ param(bool flag) { m_param = flag; }
    ଅଚଳ string& paramPath() ଅଚଳ { ଫେରନ୍ତୁ m_paramPath; }
    ଶୂନ୍ୟ paramPath(ଅଚଳ string& path) { m_paramPath = path; }
    bool svDotName() ଅଚଳ { ଫେରନ୍ତୁ m_svDotName; }
    ଶୂନ୍ୟ svDotName(bool flag) { m_svDotName = flag; }
    bool svImplicit() ଅଚଳ { ଫେରନ୍ତୁ m_svImplicit; }
    ଶୂନ୍ୟ svImplicit(bool flag) { m_svImplicit = flag; }
};
class AstPort final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A port (in/out/inout) on a module
    // @astgen op1 := exprp : Optional[AstNodeExpr] // Expression connected to port
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_pin_ଅଙ୍କ;  // Pin number
    ଅଚଳ string m_name;  // Name of pin
ସର୍ବସାଧାରଣ:
    AstPort(File_ଧାଡ଼ି* fl, ପୂର୍ଣ୍ଣସଂଖ୍ୟା pinnum, ଅଚଳ string& name)
        : ASTGEN_SUPER_Port(fl)
        , m_pin_ଅଙ୍କ{pinnum}
        , m_name{name} {}
    ASTGEN_MEMBERS_AstPort;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Port name
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା pin_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_pin_ଅଙ୍କ; }  // * = Pin number, for order based instantiation
};
class AstPragma final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    ଅଚଳ VPragmaType m_pragType;  // Type of pragma
    ଅଚଳ VTimescale m_timescale;  // For TIMEUNIT_SET
ସର୍ବସାଧାରଣ:
    // Pragmas don't result in any output code, they're just flags that affect
    // other processing in verilator.
    AstPragma(File_ଧାଡ଼ି* fl, VPragmaType pragType)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{pragType} {
        UASSERT_OBJ(pragType != VPragmaType::TIMEUNIT_SET, fl, "Should use other constructor");
    }
    AstPragma(File_ଧାଡ଼ି* fl, ଅଚଳ VTimescale& timescale)
        : ASTGEN_SUPER_Pragma(fl)
        , m_pragType{VPragmaType::TIMEUNIT_SET}
        , m_timescale{timescale} {}
    ASTGEN_MEMBERS_AstPragma;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VPragmaType pragType() ଅଚଳ { ଫେରନ୍ତୁ m_pragType; }  // *=type of the pragma
    bool isPredictOptimizable() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ pragType() == VN_DBG_AS(samep, Pragma)->pragType();
    }
    VTimescale timescale() ଅଚଳ { ଫେରନ୍ତୁ m_timescale; }
};
class AstPropSpec final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A clocked property
    // Parents:  ASSERT|COVER (property)
    // Children: SENITEM, Properties
    // @astgen op1 := sensesp : Optional[AstSenItem]
    // @astgen op2 := disablep : Optional[AstNodeExpr]
    // @astgen op3 := propp : AstNode
    // @astgen op4 := matchCountp : Optional[AstNodeExpr] // Cover sequence matches this tick
    VPropStrength m_propStrength = VPropStrength::DEFAULT;

ସର୍ବସାଧାରଣ:
    AstPropSpec(File_ଧାଡ଼ି* fl, AstSenItem* sensesp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* disablep, Ast_ଗଣ୍ଠି* propp,
                VPropStrength propStrength = VPropStrength::DEFAULT)
        : ASTGEN_SUPER_PropSpec(fl)
        , m_propStrength{propStrength} {
        this->sensesp(sensesp);
        this->disablep(disablep);
        this->propp(propp);
    }
    ASTGEN_MEMBERS_AstPropSpec;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE {
        ଫେରନ୍ତୁ true;
    }  // Used under Cover, which expects a bool child
    VPropStrength propStrength() ଅଚଳ { ଫେରନ୍ତୁ m_propStrength; }
};
class AstPull final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // @astgen op1 := lhsp : AstNodeExpr

    ଅଚଳ bool m_direction;

ସର୍ବସାଧାରଣ:
    AstPull(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp, bool direction)
        : ASTGEN_SUPER_Pull(fl)
        , m_direction{direction} {
        this->lhsp(lhsp);
    }
    ASTGEN_MEMBERS_AstPull;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ direction() == VN_DBG_AS(samep, Pull)->direction();
    }
    bool direction() ଅଚଳ { ଫେରନ୍ତୁ m_direction; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstScope final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
ସର୍ବସାଧାରଣ:
    AstScope(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_Module* modp, ଅଚଳ string& name, AstScope* aboveScopep,
             AstCell* aboveCellp)
        : ASTGEN_SUPER_Scope(fl)
        , m_name{name}
        , m_aboveScopep{aboveScopep}
        , m_aboveCellp{aboveCellp}
        , m_modp{modp} {}
    ASTGEN_MEMBERS_AstScope;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!m_modp);
        ଫେରନ୍ତୁ nullptr;
    }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Scope name
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    string pretty_Name_ସନ୍ଦେଶ() ଅଚଳ override;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    string nameDotless() ଅଚଳ;
    Ast_ଗଣ୍ଠି_Module* modp() ଅଚଳ { ଫେରନ୍ତୁ m_modp; }
    // The combinational AstActive of the scope. If none, create one if 'create', else nullptr
    AstActive* comboActivep(bool create);
    //
    AstScope* aboveScopep() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_aboveScopep; }
    ଶୂନ୍ୟ aboveScopep(AstScope* nodep) { m_aboveScopep = nodep; }
    AstCell* aboveCellp() ଅଚଳ { ଫେରନ୍ତୁ m_aboveCellp; }
    ଶୂନ୍ୟ aboveCellp(AstCell* nodep) { m_aboveCellp = nodep; }
    bool is_ଶୀର୍ଷ() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ aboveScopep() == nullptr; }  // At top of hierarchy
};
class AstSenItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Parents:  SENTREE
    // @astgen op1 := sensp : Optional[AstNodeExpr] // Sensitivity expression
    // @astgen op2 := condp : Optional[AstNodeExpr] // Sensitivity condition
    VEdgeType m_edgeType;  // Edge type
ସର୍ବସାଧାରଣ:
    class Combo {};  // for constructor type-overload selection
    class Static {};  // for constructor type-overload selection
    class Initial {};  // for constructor type-overload selection
    class InitialNBA {};  // for constructor type-overload selection
    class Final {};  // for constructor type-overload selection
    class Never {};  // for constructor type-overload selection
    AstSenItem(File_ଧାଡ଼ି* fl, VEdgeType edgeType, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* senp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp = nullptr)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{edgeType} {
        this->sensp(senp);
        this->condp(condp);
    }
    AstSenItem(File_ଧାଡ଼ି* fl, Combo)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_COMBO} {}
    AstSenItem(File_ଧାଡ଼ି* fl, Static)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_STATIC} {}
    AstSenItem(File_ଧାଡ଼ି* fl, Initial)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL} {}
    AstSenItem(File_ଧାଡ଼ି* fl, InitialNBA)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_INITIAL_NBA} {}
    AstSenItem(File_ଧାଡ଼ି* fl, Final)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_FINAL} {}
    AstSenItem(File_ଧାଡ଼ି* fl, Never)
        : ASTGEN_SUPER_SenItem(fl)
        , m_edgeType{VEdgeType::ET_NEVER} {}
    ASTGEN_MEMBERS_AstSenItem;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ edgeType() == VN_DBG_AS(samep, SenItem)->edgeType();
    }
    VEdgeType edgeType() ଅଚଳ { ଫେରନ୍ତୁ m_edgeType; }
    ଶୂନ୍ୟ edgeType(VEdgeType type) {
        m_edgeType = type;
        edit_ଗଣନା_Inc();
    }
    Ast_ଗଣ୍ଠି_Var_Ref* varrefp() ଅଚଳ { ଫେରନ୍ତୁ VN_CAST(sensp(), ଗଣ୍ଠି_Var_Ref); }
    //
    bool isClocked() ଅଚଳ { ଫେରନ୍ତୁ edgeType().clockedStmt(); }
    bool isComboOrStar() ଅଚଳ {
        ଫେରନ୍ତୁ edgeType() == VEdgeType::ET_COMBO || edgeType() == VEdgeType::ET_COMBO_STAR;
    }
    bool isComboStar() ଅଚଳ { ଫେରନ୍ତୁ edgeType() == VEdgeType::ET_COMBO_STAR; }
    bool isHybrid() ଅଚଳ { ଫେରନ୍ତୁ edgeType() == VEdgeType::ET_HYBRID; }
    bool isStatic() ଅଚଳ { ଫେରନ୍ତୁ edgeType() == VEdgeType::ET_STATIC; }
    bool isInitial() ଅଚଳ { ଫେରନ୍ତୁ edgeType() == VEdgeType::ET_INITIAL; }
    bool isFinal() ଅଚଳ { ଫେରନ୍ତୁ edgeType() == VEdgeType::ET_FINAL; }
    bool isNever() ଅଚଳ { ଫେରନ୍ତୁ edgeType() == VEdgeType::ET_NEVER; }
};
class Ast_Sen_ବୃକ୍ଷ final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A sensitivity list
    // @astgen op1 := sensesp : List[AstSenItem]
ସର୍ବସାଧାରଣ:
    Ast_Sen_ବୃକ୍ଷ(File_ଧାଡ଼ି* fl, AstSenItem* sensesp)
        : ASTGEN_SUPER_Sen_ବୃକ୍ଷ(fl) {
        addSensesp(sensesp);
    }
    ASTGEN_MEMBERS_Ast_Sen_ବୃକ୍ଷ;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    // METHODS
    bool hasClocked() ଅଚଳ;  // Includes a clocked statement
    bool hasEdge() ଅଚଳ;  // Includes a posedge/negedge/bothedge clocked statement
    bool hasStatic() ଅଚଳ;  // Includes a STATIC SenItem
    bool hasInitial() ଅଚଳ;  // Includes a INITIAL SenItem
    bool hasFinal() ଅଚଳ;  // Includes a FINAL SenItem
    bool hasCombo() ଅଚଳ;  // Includes a COMBO SenItem
    bool hasHybrid() ଅଚଳ;  // Includes a HYBRID SenItem
};
class AstStrengthSpec final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    VStrength m_s0;  // Drive 0 strength
    VStrength m_s1;  // Drive 1 strength

ସର୍ବସାଧାରଣ:
    AstStrengthSpec(File_ଧାଡ଼ି* fl, VStrength s0, VStrength s1)
        : ASTGEN_SUPER_StrengthSpec(fl)
        , m_s0{s0}
        , m_s1{s1} {}

    ASTGEN_MEMBERS_AstStrengthSpec;
    VStrength strength0() { ଫେରନ୍ତୁ m_s0; }
    VStrength strength1() { ଫେରନ୍ତୁ m_s1; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstSystemCSection final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Verilator specific "`systemc_* block". This is a "module item"
    // containing arbitrary text that is emitted to the C++ output in various
    // locations depending on the sectionType.
    ଅଚଳ VSystemCSectionType m_sectionType;  // The section type
    std::string m_text;  // The text content

ସର୍ବସାଧାରଣ:
    AstSystemCSection(File_ଧାଡ଼ି* fl, VSystemCSectionType sectionType, ଅଚଳ std::string& text)
        : ASTGEN_SUPER_SystemCSection(fl)
        , m_sectionType{sectionType}
        , m_text{text} {}
    ASTGEN_MEMBERS_AstSystemCSection;
    VSystemCSectionType sectionType() ଅଚଳ { ଫେରନ୍ତୁ m_sectionType; }
    ଅଚଳ std::string& text() ଅଚଳ { ଫେରନ୍ତୁ m_text; }
    ଶୂନ୍ୟ text(ଅଚଳ std::string& ମୂଲ୍ୟ) { m_text = ମୂଲ୍ୟ; }
    ଶୂନ୍ୟ dump(std::ostream&) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream&) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି*) ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool isPure() override { ଫେରନ୍ତୁ false; }
    bool isOutputter() override { ଫେରନ୍ତୁ true; }
};
class AstText final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Represents a piece of text to be emitted into the output
    //
    // Avoid using this directly, internally usually want
    // AstCStmt::add("text") or AstCExpr::add("text") instead
    //
    ଅଚଳ std::string m_text;  // The text to emit
ସର୍ବସାଧାରଣ:
    AstText(File_ଧାଡ଼ି* fl, ଅଚଳ std::string& text)
        : ASTGEN_SUPER_Text(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_AstText;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ text() == VN_DBG_AS(samep, Text)->text();
    }
    ଅଚଳ std::string& text() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_text; }
};
class AstTextBlock final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Text block emitted into output, with some arbitrary nodes interspersed
    // @astgen op1 := nodesp : List[AstNode] // Nodes to print
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ଅଚଳ std::string m_prefix;  // Prefix to print before first element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ଅଚଳ std::string m_separator;  // Separator to print between each element in 'nodesp'
    // dist-ast-dump-suppress  // Omitting text blocks due to verbosity
    ଅଚଳ std::string m_suffix;  // Suffix to pring after last element in 'nodesp'
ସର୍ବସାଧାରଣ:
    explicit AstTextBlock(File_ଧାଡ଼ି* fl,  //
                          ଅଚଳ std::string& prefix = "",  //
                          ଅଚଳ std::string& separator = "",  //
                          ଅଚଳ std::string& suffix = "")
        : ASTGEN_SUPER_TextBlock(fl)
        , m_prefix{prefix}
        , m_separator{separator}
        , m_suffix{suffix} {}
    ASTGEN_MEMBERS_AstTextBlock;
    ଅଚଳ std::string& prefix() ଅଚଳ { ଫେରନ୍ତୁ m_prefix; }
    ଅଚଳ std::string& separator() ଅଚଳ { ଫେରନ୍ତୁ m_separator; }
    ଅଚଳ std::string& suffix() ଅଚଳ { ଫେରନ୍ତୁ m_suffix; }
    // Add some text, or a node to this block
    ଶୂନ୍ୟ add(ଅଚଳ string& text) { addNodesp(new AstText{fileline(), text}); }
    ଶୂନ୍ୟ add(Ast_ଗଣ୍ଠି* nodep) { addNodesp(nodep); }
};
class Ast_ଶୀର୍ଷ_Scope final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // A singleton, held under the top level AstModule. Holds the top level
    // AstScope, and after V3ActiveTop, the global list of AstSenTrees (list of
    // unique sensitivity lists).
    //
    // @astgen op1 := senTreesp : List[AstSenTree] // Globally unique sensitivity lists
    // @astgen op2 := scopep : AstScope // The AstScope of the top-leveL

    friend class AstNetlist;  // Only the AstNetlist can create one
    Ast_ଶୀର୍ଷ_Scope(File_ଧାଡ଼ି* fl, AstScope* ascopep)
        : ASTGEN_SUPER_ଶୀର୍ଷ_Scope(fl) {
        this->scopep(ascopep);
    }

ସର୍ବସାଧାରଣ:
    ASTGEN_MEMBERS_Ast_ଶୀର୍ଷ_Scope;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    // Create new MODULETEMP variable in the top level scope. Name must start with '__V'
    AstVarScope* createTemp(ଅଚଳ string& name, unsigned width);
    AstVarScope* createTemp(ଅଚଳ string& name, Ast_ଗଣ୍ଠି_DType* dtypep);
};
class AstTypeTable final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
    using DetailedMap = std::map<VBasic_Type_ଚାବି, AstBasicDType*>;
    // dist-ast-dump-suppress  // Link to other nodes
    DetailedMap m_detailedMap;

ସର୍ବସାଧାରଣ:
    explicit AstTypeTable(File_ଧାଡ଼ି* fl);
    ASTGEN_MEMBERS_AstTypeTable;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ cloneRelink() override { V3ERROR_NA; }  // Not cloneable
    AstBasicDType* findBasicDType(File_ଧାଡ଼ି* fl, VBasicDTypeKwd kwd);
    AstBasicDType* findLogicBitDType(File_ଧାଡ଼ି* fl, VBasicDTypeKwd kwd, ପୂର୍ଣ୍ଣସଂଖ୍ୟା width, ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin,
                                     VSigning numeric);
    AstBasicDType* findLogicBitDType(File_ଧାଡ଼ି* fl, VBasicDTypeKwd kwd, ଅଚଳ VNumRange& range,
                                     ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthMin, VSigning numeric);
    AstBasicDType* findCreateSameDType(AstBasicDType& ଗଣ୍ଠି);
    AstBasicDType* find_ଭର୍ତ୍ତି_Same_DType(AstBasicDType* nodep);
    AstConstraintRefDType* findConstraintRefDType(File_ଧାଡ଼ି* fl);
    AstEmptyQueueDType* findEmptyQueueDType(File_ଧାଡ଼ି* fl);
    AstQueueDType* findQueueIndexDType(File_ଧାଡ଼ି* fl);
    AstStreamDType* findStreamDType(File_ଧାଡ଼ି* fl);
    AstVoidDType* findVoidDType(File_ଧାଡ଼ି* fl);
    ଶୂନ୍ୟ clearCache();
    ଶୂନ୍ୟ repairCache();
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
};
class AstTypedef final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // @astgen op1 := childDTypep : Optional[AstNodeDType]
    // @astgen op4 := attrsp : List[AstNode] // Attributes during early parse

    string m_name;  // Name of the typedef
    string m_tag;  // Holds the string of the verilator tag -- used in JSON output.
    uint32_t m_decl_Token_ଅଙ୍କ;  // Declaration token number
    bool m_attrPublic = false;  // Marked with public; keep even if unused
    bool m_isHideLocal : 1;  // Verilog local
    bool m_isHideProtected : 1;  // Verilog protected
    bool m_isUnderClass : 1;  // Underneath class

ସର୍ବସାଧାରଣ:
    AstTypedef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* attrsp, VFlagChildDType,
               Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ଅଙ୍କ{fl->token_ଅଙ୍କ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{false} {
        childDTypep(dtp);  // Only for parser
        addAttrsp(attrsp);
        dtypep(nullptr);  // V3Width will resolve
    }
    AstTypedef(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_DType* dtp, bool underClass)
        : ASTGEN_SUPER_Typedef(fl)
        , m_name{name}
        , m_decl_Token_ଅଙ୍କ{fl->token_ଅଙ୍କ()}
        , m_isHideLocal{false}
        , m_isHideProtected{false}
        , m_isUnderClass{underClass} {
        dtypep(dtp);
    }
    ASTGEN_MEMBERS_AstTypedef;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    // Name of the type it declares, with the scope declaring it, so with the values of the
    // parameters of the scope, e.g. '$unit::Cls#(8)::s_t'
    string dtypeName() ଅଚଳ;
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ dtypep() ? dtypep() : childDTypep();
    }
    // METHODS
    uint32_t decl_Token_ଅଙ୍କ() ଅଚଳ override { ଫେରନ୍ତୁ m_decl_Token_ଅଙ୍କ; }
    ଶୂନ୍ୟ decl_Token_ଅଙ୍କ_Set_Min(uint32_t token_ଅଙ୍କ) override {
        m_decl_Token_ଅଙ୍କ = std::min(m_decl_Token_ଅଙ୍କ, token_ଅଙ୍କ);
    }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ name(ଅଚଳ string& flag) override { m_name = flag; }
    bool attrPublic() ଅଚଳ { ଫେରନ୍ତୁ m_attrPublic; }
    ଶୂନ୍ୟ attrPublic(bool flag) { m_attrPublic = flag; }
    bool isHideLocal() ଅଚଳ { ଫେରନ୍ତୁ m_isHideLocal; }
    ଶୂନ୍ୟ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ଅଚଳ { ଫେରନ୍ତୁ m_isHideProtected; }
    ଶୂନ୍ୟ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool isUnderClass() ଅଚଳ { ଫେରନ୍ତୁ m_isUnderClass; }
    ଶୂନ୍ୟ isUnderClass(bool flag) { m_isUnderClass = flag; }
    ଶୂନ୍ୟ tag(ଅଚଳ string& text) override { m_tag = text; }
    string tag() ଅଚଳ override { ଫେରନ୍ତୁ m_tag; }
};
class AstTypedefFwd final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // Forward declaration of a type; stripped after netlist parsing is complete
    string m_name;
    ଅଚଳ VFwdType m_fwdType;  // Forward type for lint check

ସର୍ବସାଧାରଣ:
    AstTypedefFwd(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ VFwdType& fwdType)
        : ASTGEN_SUPER_TypedefFwd(fl)
        , m_name{name}
        , m_fwdType{fwdType} {}
    ASTGEN_MEMBERS_AstTypedefFwd;
    // METHODS
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    VFwdType fwdType() ଅଚଳ { ଫେରନ୍ତୁ m_fwdType; }
};
class AstUdpTable final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // @astgen op1 := linesp : List[AstUdpTableLine]
ସର୍ବସାଧାରଣ:
    AstUdpTable(File_ଧାଡ଼ି* fl, Ast_Udp_Table_ଧାଡ଼ି* linesp)
        : ASTGEN_SUPER_UdpTable(fl) {
        this->addLinesp(linesp);
        ଯଦି (!v3Global.hasTable()) v3Global.setHasTable();
    }
    ASTGEN_MEMBERS_AstUdpTable;
};
class Ast_Udp_Table_ଧାଡ଼ି final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    // @astgen op1 := iFieldsp : List[AstUdpTableLineVal]  // Input fields
    // @astgen op2 := oFieldsp : List[AstUdpTableLineVal]  // Output fields
ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ bool m_udpIsCombo;  // Combinational or sequential UDP

ସର୍ବସାଧାରଣ:
    class UdpCombo {};
    Ast_Udp_Table_ଧାଡ଼ି(UdpCombo, File_ଧାଡ଼ି* fl, Ast_Udp_Table_ଧାଡ଼ି_Val* iFieldsp,
                    Ast_Udp_Table_ଧାଡ଼ି_Val* oFieldsp)
        : ASTGEN_SUPER_Udp_Table_ଧାଡ଼ି(fl)
        , m_udpIsCombo{true} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp);
    }
    class UdpSequential {};
    Ast_Udp_Table_ଧାଡ଼ି(UdpSequential, File_ଧାଡ଼ି* fl, Ast_Udp_Table_ଧାଡ଼ି_Val* iFieldsp,
                    Ast_Udp_Table_ଧାଡ଼ି_Val* oFieldsp1, Ast_Udp_Table_ଧାଡ଼ି_Val* oFieldsp2)
        : ASTGEN_SUPER_Udp_Table_ଧାଡ଼ି(fl)
        , m_udpIsCombo{false} {
        addIFieldsp(iFieldsp);
        addOFieldsp(oFieldsp1);
        addOFieldsp(oFieldsp2);
    }
    ASTGEN_MEMBERS_Ast_Udp_Table_ଧାଡ଼ି;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା udpIsCombo() ଅଚଳ { ଫେରନ୍ତୁ m_udpIsCombo; }
};
class Ast_Udp_Table_ଧାଡ଼ି_Val final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
    string m_text;  // Value character

ସର୍ବସାଧାରଣ:
    Ast_Udp_Table_ଧାଡ଼ି_Val(File_ଧାଡ଼ି* fl, ଅଚଳ string& text)
        : ASTGEN_SUPER_Udp_Table_ଧାଡ଼ି_Val(fl)
        , m_text{text} {}
    ASTGEN_MEMBERS_Ast_Udp_Table_ଧାଡ଼ି_Val;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_text; }
    ଶୂନ୍ୟ name(std::string ଅଚଳ& text) override VL_MT_STABLE { m_text = text; }
    string text() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_text; }
};
class AstVar final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_pin_ଅଙ୍କ = 0;  // For JSON, if non-zero the connection pin number
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_libInoutId = 0;  // Identifies the three ports of a lowered library inout
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
    bool m_mtask_Cache_ଧାଡ଼ି_Align : 1;  // Start MTask affinity group on a cache line
    ଶୂନ୍ୟ init() {
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
        m_mtask_Cache_ଧାଡ଼ି_Align = false;
    }

ସର୍ବସାଧାରଣ:
    AstVar(File_ଧାଡ଼ି* fl, VVarType type, ଅଚଳ string& name, VFlagChildDType, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        childDTypep(dtp);  // Only for parser
        dtypep(nullptr);  // V3Width will resolve
    }
    AstVar(File_ଧାଡ଼ି* fl, VVarType type, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_DType* dtp)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        UASSERT(dtp, "AstVar created with no dtype");
        dtypep(dtp);
    }
    AstVar(File_ଧାଡ଼ି* fl, VVarType type, ଅଚଳ string& name, VFlagLogicPacked, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetLogicSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ଧାଡ଼ି* fl, VVarType type, ଅଚଳ string& name, VFlagBitPacked, ପୂର୍ଣ୍ଣସଂଖ୍ୟା wantwidth)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        dtypeSetBitSized(wantwidth, VSigning::UNSIGNED);
    }
    AstVar(File_ଧାଡ଼ି* fl, VVarType type, ଅଚଳ string& name, ଅଚଳ AstVar* examplep)
        : ASTGEN_SUPER_Var(fl)
        , m_name{name}
        , m_origName{name} {
        init();
        combineType(type);
        ଯଦି (examplep->childDTypep()) childDTypep(examplep->childDTypep()->clone_ବୃକ୍ଷ(true));
        dtypeFrom(examplep);
    }
    ASTGEN_MEMBERS_AstVar;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଧାଡ଼ିଭିତରେ bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }  // * = Var name
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    string origName() ଅଚଳ override { ଫେରନ୍ତୁ m_origName; }  // * = Original name
    ଶୂନ୍ୟ origName(ଅଚଳ string& name) { m_origName = name; }
    VVarType varType() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_varType; }  // * = Type of variable
    ଶୂନ୍ୟ direction(ଅଚଳ VDirection& flag) {
        m_direction = flag;
        ଯଦି (m_direction == VDirection::INOUT) m_tristate = true;
    }
    VDirection direction() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_direction; }
    bool isIO() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_direction != VDirection::NONE; }
    bool isVLIO() ଅଚଳ {
        ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = basicp();
        ଫେରନ୍ତୁ isPrimaryIO() && bdtypep && !bdtypep->isOpaque()
               && !dtypep()->skipRefp()->isCompound()
               && !VN_IS(dtypep()->skipRefp(), UnpackArrayDType);
    }
    ଶୂନ୍ୟ declDirection(ଅଚଳ VDirection& flag) { m_declDirection = flag; }
    VDirection declDirection() ଅଚଳ { ଫେରନ୍ତୁ m_declDirection; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା libInoutId() ଅଚଳ { ଫେରନ୍ତୁ m_libInoutId; }
    ଶୂନ୍ୟ libInoutId(ପୂର୍ଣ୍ଣସଂଖ୍ୟା id) { m_libInoutId = id; }
    bool libInoutEnable() ଅଚଳ { ଫେରନ୍ତୁ m_libInoutEnable; }
    ଶୂନ୍ୟ libInoutEnable(bool flag) { m_libInoutEnable = flag; }
    ଶୂନ୍ୟ varType(VVarType type) { m_varType = type; }
    ଶୂନ୍ୟ varType2Out() {
        m_tristate = false;
        m_direction = VDirection::OUTPUT;
    }
    ଶୂନ୍ୟ varType2In() {
        m_tristate = false;
        m_direction = VDirection::ନିବେଶ;
    }
    string scType() ଅଚଳ;  // Return SysC type: bool, uint32_t, uint64_t, sc_bv
    // Return C /*public*/ type for argument: bool, uint32_t, uint64_t, etc.
    string cPubArgType(bool named, bool forReturn) ଅଚଳ;
    // Return DPI-C type for argument; if 'cpp', C structures are qualified for C++ class scopes
    string dpiArgType(bool named, bool forReturn, bool cpp = false) ଅଚଳ;
    string dpiTmpVarType(ଅଚଳ string& varName) ଅଚଳ;
    // Return Verilator internal type for argument: CData, SData, IData, WData
    string vlArgType(bool named, bool forReturn, bool forFunc, bool asRef = false,
                     bool constRef = false) ଅଚଳ;
    string vlEnumType() ଅଚଳ;  // Return VerilatorVarType: VLVT_UINT32, etc
    string vlEnumDir(bool forMember = false) ଅଚଳ;  // Return VerilatorVarDir: VLVD_INOUT, etc
    string vlPropDecl(ଅଚଳ string& propName) ଅଚଳ;  // Return VerilatorVarProps declaration
    ଶୂନ୍ୟ combineType(VVarType type);
    Ast_ଗଣ୍ଠି_DType* getChildDTypep() ଅଚଳ override { ଫେରନ୍ତୁ childDTypep(); }
    Ast_ଗଣ୍ଠି_DType* dtypeSkipRefp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->skipRefp(); }
    // (Slow) recurse down to find basic data type (Note don't need virtual -
    // AstVar isn't a NodeDType)
    AstBasicDType* basicp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ subDTypep()->basicp(); }
    ଆଭାସୀ Ast_ଗଣ୍ଠି_DType* subDTypep() ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ dtypep() ? dtypep() : childDTypep();
    }
    ଶୂନ୍ୟ ansi(bool flag) { m_ansi = flag; }
    ଶୂନ୍ୟ declTyped(bool flag) { m_declTyped = flag; }
    ଶୂନ୍ୟ sensIfacep(AstIface* nodep) { m_sensIfacep = nodep; }
    ଶୂନ୍ୟ attrFileDescr(bool flag) { m_attrFileDescr = flag; }
    ଶୂନ୍ୟ attrScBv(bool flag) { m_attrScBv = flag; }
    ଶୂନ୍ୟ attrScBigUint(bool flag) { m_attrScBigUint = flag; }
    ଶୂନ୍ୟ attrSFormat(bool flag) { m_attrSFormat = flag; }
    ଶୂନ୍ୟ attrSplitVar(bool flag) { m_attrSplitVar = flag; }
    ଶୂନ୍ୟ attrFsmState(bool flag) { m_attrFsmState = flag; }
    ଶୂନ୍ୟ attrFsmArcInclCond(bool flag) { m_attrFsmArcInclCond = flag; }
    ଶୂନ୍ୟ attrFsmStateExpand(ଅଚଳ VFsmExpandType flag) {
        UASSERT_OBJ(flag < 4, this, "Expand mode value should be in range [0;3]");
        m_attrFsmStateExpand = flag;
    }
    bool constPoolEntry() ଅଚଳ { ଫେରନ୍ତୁ m_constPoolEntry; }
    ଶୂନ୍ୟ setConstPoolEntry() { m_constPoolEntry = true; }
    bool covergroupRefMember() ଅଚଳ { ଫେରନ୍ତୁ m_covergroupRefMember; }
    ଶୂନ୍ୟ covergroupRefMember(bool flag) { m_covergroupRefMember = flag; }
    // Instance variable an embedded covergroup declares, of the covergroup's anonymous type
    // (IEEE 1800-2023 19.4)
    bool embeddedCovergroup() ଅଚଳ { ଫେରନ୍ତୁ m_embeddedCovergroup; }
    ଶୂନ୍ୟ embeddedCovergroup(bool flag) { m_embeddedCovergroup = flag; }
    ଶୂନ୍ୟ rand(ଅଚଳ VRandAttr flag) { m_rand = flag; }
    ଶୂନ୍ୟ usedParam(bool flag) { m_usedParam = flag; }
    ଶୂନ୍ୟ usedLoopIdx(bool flag) { m_usedLoopIdx = flag; }
    ଶୂନ୍ୟ sigPublic(bool flag) { m_sigPublic = flag; }
    ଶୂନ୍ୟ sigModPublic(bool flag) { m_sigModPublic = flag; }
    ଶୂନ୍ୟ sigUserRdPublic(bool flag) {
        m_sigUserRdPublic = flag;
        ଯଦି (flag) sigPublic(true);
    }
    ଶୂନ୍ୟ sigUserRWPublic(bool flag) {
        m_sigUserRWPublic = flag;
        ଯଦି (flag) sigUserRdPublic(true);
    }
    ଶୂନ୍ୟ sc(bool flag) { m_sc = flag; }
    ଶୂନ୍ୟ scSensitive(bool flag) { m_scSensitive = flag; }
    ଶୂନ୍ୟ primaryIO(bool flag) { m_primaryIO = flag; }
    ଶୂନ୍ୟ isConst(bool flag) { m_isConst = flag; }
    ଶୂନ୍ୟ isContinuously(bool flag) { m_isContinuously = flag; }
    ଶୂନ୍ୟ isIfaceParent(bool flag) { m_isIfaceParent = flag; }
    ଶୂନ୍ୟ isIfaceArraySplit(bool flag) { m_isIfaceArraySplit = flag; }
    ଶୂନ୍ୟ isInternal(bool flag) { m_isInternal = flag; }
    ଶୂନ୍ୟ isIfaceParam(bool flag) { m_isIfaceParam = flag; }
    ଶୂନ୍ୟ funcLocal(bool flag) {
        m_funcLocal = flag;
        ଯଦି (flag) m_funcLocalSticky = true;
    }
    ଶୂନ୍ୟ funcReturn(bool flag) { m_funcReturn = flag; }
    bool gotNansiType() ଅଚଳ { ଫେରନ୍ତୁ m_gotNansiType; }
    ଶୂନ୍ୟ gotNansiType(bool flag) { m_gotNansiType = flag; }
    bool hasStrengthAssignment() ଅଚଳ { ଫେରନ୍ତୁ m_hasStrengthAssignment; }
    ଶୂନ୍ୟ hasStrengthAssignment(bool flag) { m_hasStrengthAssignment = flag; }
    bool hasUserInit() ଅଚଳ { ଫେରନ୍ତୁ m_hasUserInit; }
    ଶୂନ୍ୟ hasUserInit(bool flag) { m_hasUserInit = flag; }
    ଶୂନ୍ୟ icoMaybeWritten(bool flag) { m_icoMaybeWritten = flag; }
    bool icoMaybeWritten() ଅଚଳ { ଫେରନ୍ତୁ m_icoMaybeWritten; }
    ଶୂନ୍ୟ isClockingDriven(bool flag) { m_isClockingDriven = flag; }
    bool isClockingDriven() ଅଚଳ { ଫେରନ୍ତୁ m_isClockingDriven; }
    bool isDpiOpenArray() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_isDpiOpenArray; }
    ଶୂନ୍ୟ isDpiOpenArray(bool flag) { m_isDpiOpenArray = flag; }
    bool isHideLocal() ଅଚଳ { ଫେରନ୍ତୁ m_isHideLocal; }
    ଶୂନ୍ୟ isHideLocal(bool flag) { m_isHideLocal = flag; }
    bool isHideProtected() ଅଚଳ { ଫେରନ୍ତୁ m_isHideProtected; }
    ଶୂନ୍ୟ isHideProtected(bool flag) { m_isHideProtected = flag; }
    bool noCReset() ଅଚଳ { ଫେରନ୍ତୁ m_noCReset; }
    ଶୂନ୍ୟ noCReset(bool flag) { m_noCReset = flag; }
    bool noReset() ଅଚଳ { ଫେରନ୍ତୁ m_noReset; }
    ଶୂନ୍ୟ noReset(bool flag) { m_noReset = flag; }
    bool noSubst() ଅଚଳ { ଫେରନ୍ତୁ m_noSubst; }
    ଶୂନ୍ୟ noSubst(bool flag) { m_noSubst = flag; }
    bool processQueue() ଅଚଳ { ଫେରନ୍ତୁ m_processQueue; }
    ଶୂନ୍ୟ processQueue(bool flag) { m_processQueue = flag; }
    bool sampled() ଅଚଳ { ଫେରନ୍ତୁ m_sampled; }
    ଶୂନ୍ୟ sampled(bool flag) { m_sampled = flag; }
    bool substConstOnly() ଅଚଳ { ଫେରନ୍ତୁ m_substConstOnly; }
    ଶୂନ୍ୟ substConstOnly(bool flag) { m_substConstOnly = flag; }
    bool overriddenParam() ଅଚଳ { ଫେରନ୍ତୁ m_overriddenParam; }
    ଶୂନ୍ୟ overriddenParam(bool flag) { m_overriddenParam = flag; }
    bool untypedParam() ଅଚଳ { ଫେରନ୍ତୁ m_untypedParam; }
    ଶୂନ୍ୟ untypedParam(bool flag) { m_untypedParam = flag; }
    ଶୂନ୍ୟ trace(bool flag) { m_trace = flag; }
    ଶୂନ୍ୟ isLatched(bool flag) { m_isLatched = flag; }
    bool isForceable() ଅଚଳ { ଫେରନ୍ତୁ m_isForceable; }
    ଶୂନ୍ୟ setForceable() { m_isForceable = true; }
    ଶୂନ୍ୟ setForcedByCode() { m_isForcedByCode = true; }
    bool isForced() ଅଚଳ { ଫେରନ୍ତୁ m_isForceable || m_isForcedByCode; }
    bool isReadByDpi() ଅଚଳ { ଫେରନ୍ତୁ m_isReadByDpi; }
    ଶୂନ୍ୟ setReadByDpi() { m_isReadByDpi = true; }
    bool isWrittenByDpi() ଅଚଳ { ଫେରନ୍ତୁ m_isWrittenByDpi; }
    ଶୂନ୍ୟ setWrittenByDpi() { m_isWrittenByDpi = true; }
    bool isWrittenBySuspendable() ଅଚଳ { ଫେରନ୍ତୁ m_isWrittenBySuspendable; }
    ଶୂନ୍ୟ setWrittenBySuspendable() { m_isWrittenBySuspendable = true; }
    bool ignorePostRead() ଅଚଳ { ଫେରନ୍ତୁ m_ignorePostRead; }
    ଶୂନ୍ୟ setIgnorePostRead() { m_ignorePostRead = true; }
    bool ignorePostWrite() ଅଚଳ { ଫେରନ୍ତୁ m_ignorePostWrite; }
    ଶୂନ୍ୟ setIgnorePostWrite() { m_ignorePostWrite = true; }
    bool ignoreSchedWrite() ଅଚଳ { ଫେରନ୍ତୁ m_ignoreSchedWrite; }
    ଶୂନ୍ୟ setIgnoreSchedWrite() { m_ignoreSchedWrite = true; }
    bool dfgTriLowered() ଅଚଳ { ଫେରନ୍ତୁ m_dfgTriLowered; }
    ଶୂନ୍ୟ setDfgTriLowered() { m_dfgTriLowered = true; }
    bool dfgAllowMultidriveTri() ଅଚଳ { ଫେରନ୍ତୁ m_dfgAllowMultidriveTri; }
    ଶୂନ୍ୟ setDfgAllowMultidriveTri() { m_dfgAllowMultidriveTri = true; }
    ଶୂନ୍ୟ globalConstrained(bool flag) { m_globalConstrained = flag; }
    bool globalConstrained() ଅଚଳ { ଫେରନ୍ତୁ m_globalConstrained; }
    bool isStdRandomizeArg() ଅଚଳ { ଫେରନ୍ତୁ m_isStdRandomizeArg; }
    ଶୂନ୍ୟ setStdRandomizeArg() { m_isStdRandomizeArg = true; }
    // METHODS
    ଶୂନ୍ୟ name(ଅଚଳ string& name) override { m_name = name; }
    ଶୂନ୍ୟ tag(ଅଚଳ string& text) override { m_tag = text; }
    string tag() ଅଚଳ override { ଫେରନ୍ତୁ m_tag; }
    bool isAnsi() ଅଚଳ { ଫେରନ୍ତୁ m_ansi; }
    bool isContinuously() ଅଚଳ { ଫେରନ୍ତୁ m_isContinuously; }
    bool isDeclTyped() ଅଚଳ { ଫେରନ୍ତୁ m_declTyped; }
    bool isInout() ଅଚଳ { ଫେରନ୍ତୁ m_direction.isInout(); }
    bool isInoutOrRef() ଅଚଳ { ଫେରନ୍ତୁ m_direction.isInoutOrRef(); }
    bool is_ନିବେଶ() ଅଚଳ { ଫେରନ୍ତୁ m_direction.is_ନିବେଶ(); }
    bool isNonOutput() ଅଚଳ { ଫେରନ୍ତୁ m_direction.isNonOutput(); }
    bool isReadOnly() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_direction.isReadOnly(); }
    bool isConstRef() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_direction.isConstRef(); }
    bool isRef() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_direction.isRef(); }
    bool isWritable() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_direction.isWritable(); }
    bool isTristate() ଅଚଳ { ଫେରନ୍ତୁ m_tristate; }
    bool isPrimaryIO() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_primaryIO; }
    bool isPrimaryInish() ଅଚଳ { ଫେରନ୍ତୁ isPrimaryIO() && isNonOutput(); }
    bool isPrimaryClock() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_primaryClock; }
    ଶୂନ୍ୟ setPrimaryClock() { m_primaryClock = true; }
    bool isIfaceRef() ଅଚଳ { ଫେରନ୍ତୁ varType() == VVarType::IFACEREF; }
    ଶୂନ୍ୟ setIfaceRef() {
        m_direction = VDirection::NONE;
        m_varType = VVarType::IFACEREF;
    }
    bool isIfaceParent() ଅଚଳ { ଫେରନ୍ତୁ m_isIfaceParent; }
    bool isIfaceArraySplit() ଅଚଳ { ଫେରନ୍ତୁ m_isIfaceArraySplit; }
    bool isInternal() ଅଚଳ { ଫେରନ୍ତୁ m_isInternal; }
    bool isIfaceParam() ଅଚଳ { ଫେରନ୍ତୁ m_isIfaceParam; }
    bool isSignal() ଅଚଳ { ଫେରନ୍ତୁ varType().isSignal(); }
    bool isNet() ଅଚଳ { ଫେରନ୍ତୁ varType().isNet(); }
    bool isWor() ଅଚଳ { ଫେରନ୍ତୁ varType().isWor(); }
    bool isWiredNet() ଅଚଳ { ଫେରନ୍ତୁ varType().isWiredNet(); }
    bool isTemp() ଅଚଳ { ଫେରନ୍ତୁ varType().isTemp(); }
    bool isClassMember() ଅଚଳ { ଫେରନ୍ତୁ varType() == VVarType::MEMBER; }
    bool isVirtIface() ଅଚଳ {
        ଯଦି (AstIfaceRefDType* ଅଚଳ dtp = VN_CAST(dtypep(), IfaceRefDType)) {
            ଫେରନ୍ତୁ dtp->isVirtual();
        }
        ଫେରନ୍ତୁ false;
    }
    bool isStatementTemp() ଅଚଳ { ଫେରନ୍ତୁ varType() == VVarType::STMTTEMP; }
    bool isXTemp() ଅଚଳ { ଫେରନ୍ତୁ varType() == VVarType::XTEMP; }
    bool isParam() ଅଚଳ { ଫେରନ୍ତୁ varType().isParam(); }
    bool isGParam() ଅଚଳ { ଫେରନ୍ତୁ varType() == VVarType::GPARAM; }
    bool isGenVar() ଅଚଳ { ଫେରନ୍ତୁ varType() == VVarType::GENVAR; }
    // Return whether this variable is emitted as a member of its module's C++ class
    bool isModelState() ଅଚଳ {
        ଫେରନ୍ତୁ isIO() || isSignal() || isClassMember() || isTemp() || isGenVar();
    }
    // Variable needs no C++ constructor/destructor function run when default initialized/deleted
    bool isTriviallyConstructible() ଅଚଳ {
        ଫେରନ୍ତୁ !isSc() && dtypep()->isTriviallyConstructible();
    }
    bool isBitLogic() ଅଚଳ {
        ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = basicp();
        ଫେରନ୍ତୁ bdtypep && bdtypep->isBitLogic();
    }
    bool isUsedParam() ଅଚଳ { ଫେରନ୍ତୁ m_usedParam; }
    bool isUsedLoopIdx() ଅଚଳ { ଫେରନ୍ତୁ m_usedLoopIdx; }
    bool mtask_Cache_ଧାଡ଼ି_Align() ଅଚଳ { ଫେରନ୍ତୁ m_mtask_Cache_ଧାଡ଼ି_Align; }
    ଶୂନ୍ୟ mtask_Cache_ଧାଡ଼ି_Align(bool flag) { m_mtask_Cache_ଧାଡ଼ି_Align = flag; }
    bool isSc() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_sc; }
    bool isScQuad() ଅଚଳ;
    bool isScBv() ଅଚଳ VL_MT_STABLE;
    bool isScUint() ଅଚଳ;
    bool isScUintBool() ଅଚଳ;
    bool isScBigUint() ଅଚଳ VL_MT_STABLE;
    bool isScSensitive() ଅଚଳ { ଫେରନ୍ତୁ m_scSensitive; }
    bool isSigPublic() ଅଚଳ;
    bool isSigModPublic() ଅଚଳ { ଫେରନ୍ତୁ m_sigModPublic && !isIfaceRef(); }
    bool isSigUserRdPublic() ଅଚଳ { ଫେରନ୍ତୁ m_sigUserRdPublic && !isIfaceRef(); }
    bool isSigUserRWPublic() ଅଚଳ { ଫେରନ୍ତୁ m_sigUserRWPublic && !isIfaceRef(); }
    bool isTrace() ଅଚଳ { ଫେରନ୍ତୁ m_trace; }
    bool isRand() ଅଚଳ { ଫେରନ୍ତୁ m_rand.isRand(); }
    bool isRandC() ଅଚଳ { ଫେରନ୍ତୁ m_rand.isRandC(); }
    bool isConst() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_isConst; }
    bool isLatched() ଅଚଳ { ଫେରନ୍ତୁ m_isLatched; }
    bool isFuncLocal() ଅଚଳ { ଫେରନ୍ତୁ m_funcLocal; }
    bool isFuncLocalSticky() ଅଚଳ { ଫେରନ୍ତୁ m_funcLocalSticky; }
    bool isFuncReturn() ଅଚଳ { ଫେରନ୍ତୁ m_funcReturn; }
    bool isPullup() ଅଚଳ { ଫେରନ୍ତୁ m_isPullup; }
    bool isPulldown() ଅଚଳ { ଫେରନ୍ତୁ m_isPulldown; }
    bool attrScBv() ଅଚଳ { ଫେରନ୍ତୁ m_attrScBv; }
    bool attrScBigUint() ଅଚଳ { ଫେରନ୍ତୁ m_attrScBigUint; }
    bool attrFileDescr() ଅଚଳ { ଫେରନ୍ତୁ m_attrFileDescr; }
    bool attrSFormat() ଅଚଳ { ଫେରନ୍ତୁ m_attrSFormat; }
    bool attrSplitVar() ଅଚଳ { ଫେରନ୍ତୁ m_attrSplitVar; }
    bool attrFsmState() ଅଚଳ { ଫେରନ୍ତୁ m_attrFsmState; }
    bool attrFsmArcInclCond() ଅଚଳ { ଫେରନ୍ତୁ m_attrFsmArcInclCond; }
    VFsmExpandType attrFsmStateExpand() ଅଚଳ { ଫେରନ୍ତୁ m_attrFsmStateExpand; }
    AstIface* sensIfacep() ଅଚଳ { ଫେରନ୍ତୁ m_sensIfacep; }
    VRandAttr rand() ଅଚଳ { ଫେରନ୍ତୁ m_rand; }
    string verilogKwd() ଅଚଳ override;
    ଶୂନ୍ୟ lifetime(ଅଚଳ VLifetime& flag) { m_lifetime = flag; }
    VLifetime lifetime() ଅଚଳ { ଫେରନ୍ତୁ m_lifetime; }
    ଶୂନ୍ୟ pin_ଅଙ୍କ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା id) { m_pin_ଅଙ୍କ = id; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା pin_ଅଙ୍କ() ଅଚଳ { ଫେରନ୍ତୁ m_pin_ଅଙ୍କ; }
    ଶୂନ୍ୟ propagateAttrFrom(ଅଚଳ AstVar* fromp) {
        // This is getting connected to fromp; keep attributes
        // Note the method below too
        ଯଦି (fromp->attrFileDescr()) attrFileDescr(true);
        ଯଦି (fromp->isContinuously()) isContinuously(true);
    }
    ଶୂନ୍ୟ propagateSplitAttrFrom(ଅଚଳ AstVar* fromp) {
        // Splitting a variable (element/member/fragment); keep attributes
        propagateAttrFrom(fromp);
        lifetime(fromp->lifetime());
        funcLocal(fromp->isFuncLocal());
        noReset(fromp->noReset());
        noCReset(fromp->noCReset());
        attrSplitVar(fromp->attrSplitVar());
    }
    // Reason why the properties of the variable prevent splitting it, nullptr if none
    ଅଚଳ ଅକ୍ଷର* cannotSplitKindReason() ଅଚଳ;
    ଶୂନ୍ୟ propagateWrapAttrFrom(ଅଚଳ AstVar* fromp) {
        // Creating a function wrapper; keep attributes
        propagateAttrFrom(fromp);
        direction(fromp->direction());
        declDirection(fromp->declDirection());
        lifetime(fromp->lifetime());
    }
    ଶୂନ୍ୟ combineType(ଅଚଳ AstVar* otherp);
    bool needsCReset() ଅଚଳ {
        ଫେରନ୍ତୁ !isIfaceParent() && !isIfaceRef() && !noReset() && !isParam() && !isStatementTemp()
               && !noCReset() && !(basicp() && basicp()->isEvent());
    }
    ସ୍ଥିର AstVar* scVarRecurse(Ast_ଗଣ୍ଠି* nodep);
};
class AstVarScope final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି {
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
    bool m_fork_By_ମୂଲ୍ୟ : 1;  // Set before a fork, whose processes get it by value
ସର୍ବସାଧାରଣ:
    AstVarScope(File_ଧାଡ଼ି* fl, AstScope* scopep, AstVar* varp)
        : ASTGEN_SUPER_VarScope(fl)
        , m_scopep{scopep}
        , m_varp{varp} {
        UASSERT_OBJ(scopep, fl, "Scope must be non-null");
        UASSERT_OBJ(varp, fl, "Var must be non-null");
        m_trace = true;
        m_optimizeLifePost = false;
        m_fork_By_ମୂଲ୍ୟ = false;
        dtypeFrom(varp);
    }
    ASTGEN_MEMBERS_AstVarScope;
    ଶୂନ୍ୟ cloneRelink() override {
        ଯଦି (m_varp && m_varp->clonep()) {
            UASSERT(m_scopep->clonep(), "No clone cross link: " << this);
        }
        cloneRelinkGen();
    }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ scopep()->name() + "->" + varp()->name(); }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    AstVar* varp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_varp; }  // [After Link] Pointer to variable
    ଶୂନ୍ୟ varp(AstVar* nodep) { m_varp = nodep; }
    AstScope* scopep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_scopep; }  // Pointer to scope it's under
    ଶୂନ୍ୟ scopep(AstScope* nodep) { m_scopep = nodep; }
    bool isTrace() ଅଚଳ { ଫେରନ୍ତୁ m_trace; }
    ଶୂନ୍ୟ trace(bool flag) { m_trace = flag; }
    bool optimizeLifePost() ଅଚଳ { ଫେରନ୍ତୁ m_optimizeLifePost; }
    ଶୂନ୍ୟ optimizeLifePost(bool flag) { m_optimizeLifePost = flag; }
    bool fork_By_ମୂଲ୍ୟ() ଅଚଳ { ଫେରନ୍ତୁ m_fork_By_ମୂଲ୍ୟ; }
    ଶୂନ୍ୟ fork_By_ମୂଲ୍ୟ(bool flag) { m_fork_By_ମୂଲ୍ୟ = flag; }
};

// === AstNodeConsItem ===
class AstConsArrayItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons_Item {
    // An element of an AstConsArray, setting the element at the index to the value
    // Parents: CONSARRAY
    ଅଚଳ uint64_t m_index;  // Index of the element, with the array's lo() at 0
ସର୍ବସାଧାରଣ:
    AstConsArrayItem(File_ଧାଡ଼ି* fl, uint64_t index, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_ConsArrayItem(fl, valuep)
        , m_index{index} {}
    ASTGEN_MEMBERS_AstConsArrayItem;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsArray) && !VN_IS(backp(), ConsArrayItem));
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଫେରନ୍ତୁ m_index == VN_DBG_AS(samep, ConsArrayItem)->m_index;
    }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }  // By AstConsArray's map
    uint64_t index() ଅଚଳ { ଫେରନ୍ତୁ m_index; }
};
class AstConsAssocItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons_Item {
    // An element of an AstConsAssoc, setting the key to the value
    // Parents: CONSASSOC
    // @astgen op1 := keyp : AstNodeExpr // Const, or Cons for an aggregate key type
ସର୍ବସାଧାରଣ:
    AstConsAssocItem(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* keyp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_ConsAssocItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsAssocItem;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsAssoc) && !VN_IS(backp(), ConsAssocItem));
        ଫେରନ୍ତୁ nullptr;
    }
};
class AstConsDynArrayItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons_Item {
    // An element of an AstConsDynArray: a value, an array to splice, or with sizep, the
    // value at index 'keyp'
    // Parents: CONSDYNARRAY
    // @astgen op1 := keyp : Optional[AstConst]
ସର୍ବସାଧାରଣ:
    AstConsDynArrayItem(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {}
    AstConsDynArrayItem(File_ଧାଡ଼ି* fl, AstConst* keyp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_ConsDynArrayItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsDynArrayItem;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsDynArray) && !VN_IS(backp(), ConsDynArrayItem));
        ଫେରନ୍ତୁ nullptr;
    }
};
class AstConsQueueItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons_Item {
    // An element of an AstConsQueue, a value or an array to splice
    // Parents: CONSQUEUE
ସର୍ବସାଧାରଣ:
    AstConsQueueItem(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_ConsQueueItem(fl, valuep) {}
    ASTGEN_MEMBERS_AstConsQueueItem;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsQueue) && !VN_IS(backp(), ConsQueueItem));
        ଫେରନ୍ତୁ nullptr;
    }
};
class AstConsStructItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons_Item {
    // An element of an AstConsStruct, the value of a member
    // Parents: CONSSTRUCT
    // @astgen ptr := m_memberp : AstMemberDType  // The member set
ସର୍ବସାଧାରଣ:
    AstConsStructItem(File_ଧାଡ଼ି* fl, AstMemberDType* memberp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_ConsStructItem(fl, valuep)
        , m_memberp{memberp} {}
    ASTGEN_MEMBERS_AstConsStructItem;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsStruct) && !VN_IS(backp(), ConsStructItem));
        ଫେରନ୍ତୁ nullptr;
    }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    AstMemberDType* memberp() ଅଚଳ { ଫେରନ୍ତୁ m_memberp; }
};
class AstConsWildcardItem final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cons_Item {
    // An element of an AstConsWildcard, setting the key to the value
    // Parents: CONSWILDCARD
    // @astgen op1 := keyp : AstConst
ସର୍ବସାଧାରଣ:
    AstConsWildcardItem(File_ଧାଡ଼ି* fl, AstConst* keyp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep)
        : ASTGEN_SUPER_ConsWildcardItem(fl, valuep) {
        this->keyp(keyp);
    }
    ASTGEN_MEMBERS_AstConsWildcardItem;
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        // backp is the cons node or the previous element
        BROKEN_RTN(!VN_IS(backp(), ConsWildcard) && !VN_IS(backp(), ConsWildcardItem));
        ଫେରନ୍ତୁ nullptr;
    }
};

// === AstNodeCoverDecl ===
class AstCoverOtherDecl final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cover_Decl {
    // Coverage analysis point declaration
    // Used for other than toggle types of coverage
    string m_linescov;
    string m_fsmVar;  // FSM state variable name
    string m_fsmFrom;  // FSM source state label
    string m_fsmTo;  // FSM destination state label
    string m_fsmTag;  // FSM arc kind tag (e.g. reset, reset_include, default)
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_offset;  // Offset column numbers to uniq-ify IFs
ସର୍ବସାଧାରଣ:
    AstCoverOtherDecl(File_ଧାଡ଼ି* fl, ଅଚଳ string& page, ଅଚଳ string& comment,
                      ଅଚଳ string& linescov, ପୂର୍ଣ୍ଣସଂଖ୍ୟା offset, ଅଚଳ string& fsmVar = "",
                      ଅଚଳ string& fsmFrom = "", ଅଚଳ string& fsmTo = "",
                      ଅଚଳ string& fsmTag = "")
        : ASTGEN_SUPER_CoverOtherDecl(fl, page, comment)
        , m_linescov{linescov}
        , m_fsmVar{fsmVar}
        , m_fsmFrom{fsmFrom}
        , m_fsmTo{fsmTo}
        , m_fsmTag{fsmTag}
        , m_offset{offset} {}
    ASTGEN_MEMBERS_AstCoverOtherDecl;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା offset() ଅଚଳ { ଫେରନ୍ତୁ m_offset; }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା size() ଅଚଳ override { ଫେରନ୍ତୁ 1; }
    ଅଚଳ string& linescov() ଅଚଳ { ଫେରନ୍ତୁ m_linescov; }
    ଅଚଳ string& fsmVar() ଅଚଳ { ଫେରନ୍ତୁ m_fsmVar; }
    ଅଚଳ string& fsmFrom() ଅଚଳ { ଫେରନ୍ତୁ m_fsmFrom; }
    ଅଚଳ string& fsmTo() ଅଚଳ { ଫେରନ୍ତୁ m_fsmTo; }
    ଅଚଳ string& fsmTag() ଅଚଳ { ଫେରନ୍ତୁ m_fsmTag; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstCoverOtherDecl* ଅଚଳ asamep = VN_DBG_AS(samep, CoverOtherDecl);
        ଫେରନ୍ତୁ Super::same_ଗଣ୍ଠି(samep) && linescov() == asamep->linescov();
    }
};
class AstCoverToggleDecl final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Cover_Decl {
    // Coverage analysis point declaration
    // Used for toggle coverage
    ଅଚଳ VNumRange m_range;  // Packed array range covering each toggle bit
ସର୍ବସାଧାରଣ:
    AstCoverToggleDecl(File_ଧାଡ଼ି* fl, ଅଚଳ string& page, ଅଚଳ string& comment,
                       ଅଚଳ VNumRange& range)
        : ASTGEN_SUPER_CoverToggleDecl(fl, page, comment)
        , m_range{range} {}
    ASTGEN_MEMBERS_AstCoverToggleDecl;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା size() ଅଚଳ override {
        // Changes 0 -> 1 and 1 -> 0 are counted separately
        ଫେରନ୍ତୁ 2 * m_range.elements();
    }
    ଅଚଳ VNumRange& range() ଅଚଳ { ଫେରନ୍ତୁ m_range; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstCoverToggleDecl* ଅଚଳ asamep = VN_DBG_AS(samep, CoverToggleDecl);
        ଫେରନ୍ତୁ Super::same_ଗଣ୍ଠି(samep) && range() == asamep->range();
    }
};

// === AstNodeFTask ===
class AstFunc final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask {
    // A function inside a module
ସର୍ବସାଧାରଣ:
    AstFunc(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* stmtp, Ast_ଗଣ୍ଠି* fvarp)
        : ASTGEN_SUPER_Func(fl, name, stmtp) {
        this->fvarp(fvarp);
    }
    ASTGEN_MEMBERS_AstFunc;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_FTask* cloneType(ଅଚଳ string& name) override {
        ଫେରନ୍ତୁ new AstFunc{fileline(), name, nullptr, nullptr};
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "function"; }
};
class AstLet final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask {
    // Verilog "let" statement
    // Parents: MODULE
    // stmtp list first item is returned StmtExpr, as Let always returns AstNodeExpr
ସର୍ବସାଧାରଣ:
    AstLet(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_Let(fl, name, nullptr) {}
    ASTGEN_MEMBERS_AstLet;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଅଚଳ ଅକ୍ଷର* broken() ଅଚଳ override {
        BROKEN_RTN(!VN_IS(stmtsp(), Stmt_ପଦସମଷ୍ଟି));
        ଫେରନ୍ତୁ nullptr;
    }
    Ast_ଗଣ୍ଠି_FTask* cloneType(ଅଚଳ string& name) override { ଫେରନ୍ତୁ new AstLet{fileline(), name}; }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "let"; }
};
class AstProperty final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask {
    // A property inside a module
ସର୍ବସାଧାରଣ:
    AstProperty(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* stmtp)
        : ASTGEN_SUPER_Property(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstProperty;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_FTask* cloneType(ଅଚଳ string& name) override {
        ଫେରନ୍ତୁ new AstProperty{fileline(), name, nullptr};
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "property"; }
};
class AstSequence final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask {
    // A sequence inside a module
    // TODO when supported might not want to be a NodeFTask
    bool m_referenced = false;  // Set by V3LinkResolve when referenced; cleared by
                                // V3AssertPre after inlining; if still set after
                                // V3AssertPre, the reference is in an unsupported context
ସର୍ବସାଧାରଣ:
    AstSequence(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* stmtp)
        : ASTGEN_SUPER_Sequence(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstSequence;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool hasDType() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    Ast_ଗଣ୍ଠି_FTask* cloneType(ଅଚଳ string& name) override {
        ଫେରନ୍ତୁ new AstSequence{fileline(), name, nullptr};
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "sequence"; }
    bool isReferenced() ଅଚଳ { ଫେରନ୍ତୁ m_referenced; }
    ଶୂନ୍ୟ isReferenced(bool flag) { m_referenced = flag; }
};
class AstTask final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_FTask {
    // A task inside a module
ସର୍ବସାଧାରଣ:
    AstTask(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* stmtp)
        : ASTGEN_SUPER_Task(fl, name, stmtp) {}
    ASTGEN_MEMBERS_AstTask;
    Ast_ଗଣ୍ଠି_FTask* cloneType(ଅଚଳ string& name) override {
        ଫେରନ୍ତୁ new AstTask{fileline(), name, nullptr};
    }
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "task"; }
};

// === AstNodeFile ===
class AstCFile final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_File {
    // C++ output file
    // Parents:  NETLIST
    uint64_t m_complexityScore = 0;
    bool m_slow : 1;  ///< Compile w/o optimization
    bool m_source : 1;  ///< Source file (vs header file)
    bool m_support : 1;  ///< Support file (non systemc)
ସର୍ବସାଧାରଣ:
    AstCFile(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_CFile(fl, name)
        , m_slow{false}
        , m_source{false}
        , m_support{false} {}
    ASTGEN_MEMBERS_AstCFile;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
    uint64_t complexityScore() ଅଚଳ { ଫେରନ୍ତୁ m_complexityScore; }
    ଶୂନ୍ୟ complexityScore(uint64_t newScore) { m_complexityScore = newScore; }
    bool slow() ଅଚଳ { ଫେରନ୍ତୁ m_slow; }
    ଶୂନ୍ୟ slow(bool flag) { m_slow = flag; }
    bool source() ଅଚଳ { ଫେରନ୍ତୁ m_source; }
    ଶୂନ୍ୟ source(bool flag) { m_source = flag; }
    bool support() ଅଚଳ { ଫେରନ୍ତୁ m_support; }
    ଶୂନ୍ୟ support(bool flag) VL_MT_SAFE { m_support = flag; }
};
class AstVFile final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_File {
    // Verilog output file
    // Parents:  NETLIST
ସର୍ବସାଧାରଣ:
    AstVFile(File_ଧାଡ଼ି* fl, ଅଚଳ string& name)
        : ASTGEN_SUPER_VFile(fl, name) {}
    ASTGEN_MEMBERS_AstVFile;
    ଶୂନ୍ୟ dump(std::ostream& str = std::cout) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str = std::cout) ଅଚଳ override;
};
class AstCoverCross final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Func_Cov_Item {
    // @astgen op1 := itemsp   : List[AstCoverpointRef]
    // @astgen op2 := optionsp : List[AstCoverOption]     // post-LinkParse only
    // @astgen op3 := binsp    : List[AstNode]  // Parse: mixed cross bins/options;
    //                                          // post-LinkParse: AstCoverCrossBin only
    // @astgen op4 := iffp     : Optional[AstNodeExpr]  // Conditional sampling guard
ସର୍ବସାଧାରଣ:
    AstCoverCross(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, AstCoverpointRef* itemsp,
                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* iffp = nullptr)
        : ASTGEN_SUPER_CoverCross(fl, name) {
        UASSERT(itemsp, "AstCoverCross requires at least one coverpoint reference");
        addItemsp(itemsp);
        this->iffp(iffp);
    }
    ASTGEN_MEMBERS_AstCoverCross;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstCoverpoint final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Func_Cov_Item {
    // @astgen op1 := exprp : AstNodeExpr
    // @astgen op2 := binsp : List[AstNode]  // Parse: mixed AstCoverBin/AstCgOptionAssign;
    // post-LinkParse: AstCoverBin only
    // @astgen op3 := iffp : Optional[AstNodeExpr]
    // @astgen op4 := optionsp : List[AstCoverOption]
ସର୍ବସାଧାରଣ:
    AstCoverpoint(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp,
                  Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* iffp = nullptr, Ast_ଗଣ୍ଠି* binsp = nullptr)
        : ASTGEN_SUPER_Coverpoint(fl, name) {
        this->exprp(exprp);
        this->iffp(iffp);
        addBinsp(binsp);
    }
    ASTGEN_MEMBERS_AstCoverpoint;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};

// === AstNodeGen ===
class AstGenBlock final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Gen {
    // Generate 'begin'
    // @astgen op1 := genforp : Optional[AstNode]
    // @astgen op2 := itemsp : List[AstNode]
    std::string m_name;  // Name of block
    ଅଚଳ bool m_unnamed;  // Originally unnamed (name change does not affect this)
    ଅଚଳ bool m_implied;  // Not inserted by user
    AstDefaultDisable* m_defaultDisablep = nullptr;  // Default disable iff in this scope

ସର୍ବସାଧାରଣ:
    AstGenBlock(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, Ast_ଗଣ୍ଠି* itemsp, bool implied)
        : ASTGEN_SUPER_GenBlock(fl)
        , m_name{name}
        , m_unnamed{name.empty()}
        , m_implied{implied} {
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenBlock;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    std::string name() ଅଚଳ override VL_MT_STABLE { ଫେରନ୍ତୁ m_name; }
    ଶୂନ୍ୟ name(ଅଚଳ std::string& name) override { m_name = name; }
    bool unnamed() ଅଚଳ { ଫେରନ୍ତୁ m_unnamed; }
    bool implied() ଅଚଳ { ଫେରନ୍ତୁ m_implied; }
    AstDefaultDisable* defaultDisablep() ଅଚଳ { ଫେରନ୍ତୁ m_defaultDisablep; }
    ଶୂନ୍ୟ defaultDisablep(AstDefaultDisable* nodep) { m_defaultDisablep = nodep; }
    // As AstNodeModule::dtypeNameInnerScope(), e.g. 'top.gen[0].'
    string dtypeNameInnerScope(ଅଚଳ string& outer) ଅଚଳ;
};
class AstGenCase final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Gen {
    // Generate 'case'
    // @astgen op1 := exprp : AstNodeExpr // Condition (scurtinee) expression
    // @astgen op2 := itemsp : List[AstGenCaseItem]
ସର୍ବସାଧାରଣ:
    AstGenCase(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, AstGenCaseItem* itemsp)
        : ASTGEN_SUPER_GenCase(fl) {
        this->exprp(exprp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenCase;
};
class AstGenFor final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Gen {
    // Generate 'for'
    // @astgen op1 := initsp : List[AstNode]
    // @astgen op2 := condp : AstNodeExpr
    // @astgen op3 := incsp : List[AstNode]
    // @astgen op4 := itemsp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstGenFor(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* initsp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି* incsp, Ast_ଗଣ୍ଠି* itemsp)
        : ASTGEN_SUPER_GenFor(fl) {
        this->addInitsp(initsp);
        this->condp(condp);
        this->addIncsp(incsp);
        this->addItemsp(itemsp);
    }
    ASTGEN_MEMBERS_AstGenFor;
};
class AstGenIf final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Gen {
    // Generate 'if'
    // @astgen op1 := condp : AstNodeExpr
    // @astgen op2 := thensp : List[AstNode]
    // @astgen op3 := elsesp : List[AstNode]
ସର୍ବସାଧାରଣ:
    AstGenIf(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* condp, Ast_ଗଣ୍ଠି* thensp, Ast_ଗଣ୍ଠି* elsesp)
        : ASTGEN_SUPER_GenIf(fl) {
        this->condp(condp);
        this->addThensp(thensp);
        this->addElsesp(elsesp);
    }
    ASTGEN_MEMBERS_AstGenIf;
};

// === AstNodeModule ===
class AstClass final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Module {
    // @astgen op4 := extendsp : List[AstClassExtends]
    // MEMBERS
    // @astgen ptr := m_classOrPackagep : Optional[AstClassPackage]  // Package to be emitted with
    // @astgen ptr := m_covergroupEnclosingClassp : Optional[AstClass]  // Lexical enclosing class
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameters may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameters may then be gone
    uint32_t m_decl_Token_ଅଙ୍କ;  // Declaration token number
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
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_cgAutoBinMax = -1;  // option.auto_bin_max value (-1 = not set, use default 64)

    // dtypeName() as computed from the tree, with 'scope', the prefix for the scope declaring the
    // class
    string dtypeNameCalc(bool full, ଅଚଳ string& scope) ଅଚଳ;
    string dtypeNameScope() ଅଚଳ;  // Prefix of dtypeName(true) for the scope declaring the class

ସର୍ବସାଧାରଣ:
    AstClass(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : ASTGEN_SUPER_Class(fl, name, libname)
        , m_decl_Token_ଅଙ୍କ{fl->token_ଅଙ୍କ()} {}
    ASTGEN_MEMBERS_AstClass;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ isCovergroup() ? "covergroup" : "class"; }
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ true; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    bool timescaleMatters() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    AstClassPackage* classOrPackagep() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ m_classOrPackagep; }
    ଶୂନ୍ୟ classOrPackagep(AstClassPackage* classpackagep) { m_classOrPackagep = classpackagep; }
    AstClass* covergroupEnclosingClassp() ଅଚଳ VL_MT_STABLE {
        ଫେରନ୍ତୁ m_covergroupEnclosingClassp;
    }
    ଶୂନ୍ୟ covergroupEnclosingClassp(AstClass* classp) { m_covergroupEnclosingClassp = classp; }
    Ast_ଗଣ୍ଠି* membersp() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ stmtsp(); }
    ଶୂନ୍ୟ addMembersp(Ast_ଗଣ୍ଠି* nodep) { addStmtsp(nodep); }
    bool hasRandVarsUpdate() ଅଚଳ { ଫେରନ୍ତୁ m_hasRandVarsUpdate; }
    ଶୂନ୍ୟ hasRandVarsUpdate(bool flag) { m_hasRandVarsUpdate = flag; }
    bool isCovergroup() ଅଚଳ { ଫେରନ୍ତୁ m_covergroup; }
    ଶୂନ୍ୟ isCovergroup(bool flag) { m_covergroup = flag; }
    bool isExtended() ଅଚଳ { ଫେରନ୍ତୁ m_extended; }
    ଶୂନ୍ୟ isExtended(bool flag) { m_extended = flag; }
    bool isInterfaceClass() ଅଚଳ { ଫେରନ୍ତୁ m_interfaceClass; }
    ଶୂନ୍ୟ isInterfaceClass(bool flag) { m_interfaceClass = flag; }
    bool isVirtual() ଅଚଳ { ଫେରନ୍ତୁ m_virtual; }
    ଶୂନ୍ୟ isVirtual(bool flag) { m_virtual = flag; }
    bool needRNG() ଅଚଳ { ଫେରନ୍ତୁ m_needRNG; }
    ଶୂନ୍ୟ needRNG(bool flag) { m_needRNG = flag; }
    bool useVirtualPublic() ଅଚଳ { ଫେରନ୍ତୁ m_useVirtualPublic; }
    ଶୂନ୍ୟ useVirtualPublic(bool flag) { m_useVirtualPublic = flag; }
    ଶୂନ୍ୟ markPrintedFrom() { m_printedFrom = true; }
    bool isPrintedFrom() ଅଚଳ { ଫେରନ୍ତୁ m_printedFrom; }
    // Name of the class as a data type, with the values of its parameters, e.g. 'Cls#(int,5)'.
    // With 'full', as for $typename (IEEE 1800-2023 20.6.1), prefixed with the scope declaring
    // the class, e.g. '$unit::Cls#(int,5)', and with the types of parameters in full.
    string dtypeName(bool full) ଅଚଳ;
    // dtypeName(true), given 'scope', the prefix for the scope declaring the class, see
    // AstNodeModule::dtypeNameInnerScope()
    string dtypeNameIn(ଅଚଳ string& scope) ଅଚଳ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ଶୂନ୍ୟ dtypeNameFreeze();
    // Whether dtypeName() is fixed, as for a class elaborated from the design
    bool dtypeNameFrozen() ଅଚଳ { ଫେରନ୍ତୁ !m_dtypeNameFull.empty(); }
    // Named by dtypeName(), as name() is internal for a specialization, unless still a template
    string pretty_Name_ସନ୍ଦେଶ() ଅଚଳ override { ଫେରନ୍ତୁ hasGParam() ? prettyName() : dtypeName(false); }
    // Name of the anonymous type of embedded covergroup 'name', whose instance variable has the
    // covergroup's name (IEEE 1800-2023 19.4), see covergroupEnclosingClassp()
    ସ୍ଥିର string embeddedCovergroupTypeName(ଅଚଳ string& name) { ଫେରନ୍ତୁ "__vlAnonCG_" + name; }
    // Name of the covergroup that this, an embedded covergroup's type, is of
    string embeddedCovergroupName() ଅଚଳ;
    // Covergroup options accessors
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା cgAutoBinMax() ଅଚଳ { ଫେରନ୍ତୁ m_cgAutoBinMax; }
    ଶୂନ୍ୟ cgAutoBinMax(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ମୂଲ୍ୟ) { m_cgAutoBinMax = ମୂଲ୍ୟ; }
    // Return true if this class is an extension of base class (SLOW)
    // Accepts nullptrs
    ସ୍ଥିର bool isClassExtendedFrom(ଅଚଳ AstClass* refClassp, ଅଚଳ AstClass* baseClassp);
    uint32_t decl_Token_ଅଙ୍କ() ଅଚଳ override { ଫେରନ୍ତୁ m_decl_Token_ଅଙ୍କ; }
    ଶୂନ୍ୟ decl_Token_ଅଙ୍କ_Set_Min(uint32_t token_ଅଙ୍କ) override {
        m_decl_Token_ଅଙ୍କ = std::min(m_decl_Token_ଅଙ୍କ, token_ଅଙ୍କ);
    }
    ଶୂନ୍ୟ baseOverride(ଅଚଳ VBaseOverride& flag) { m_baseOverride = flag; }
    VBaseOverride baseOverride() ଅଚଳ { ଫେରନ୍ତୁ m_baseOverride; }
    // Return the lowest class extended from, or this class
    AstClass* baseMostClassp();
    ସ୍ଥିର bool isCacheableChild(ଅଚଳ Ast_ଗଣ୍ଠି* nodep);
    // Iterates top level members of the class, taking into account inheritance (starting from the
    // root superclass). Note: after V3Scope, several children are moved under an AstScope and will
    // not be found by this.
    template <typename T_Callable>
    ଶୂନ୍ୟ foreachMember(ଅଚଳ T_Callable& f) {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable<T_Callable, AstClass*, T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
            "T_Callable 'f' must have a signature compatible with 'void(AstClass*, T_Node*)', "
            "with 'T_Node' being a subtype of 'AstNode'");
        ଯଦି (ଅଚଳ AstClassExtends* ଅଚଳ cextendsp = this->extendsp()) {
            cextendsp->classp()->foreachMember(f);
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (Ast_ଗଣ୍ଠି::privateTypeTest<T_ଗଣ୍ଠି>(stmtp)) f(this, static_cast<T_ଗଣ୍ଠି*>(stmtp));
        }
    }
    // Same as above, but stops after first match
    template <typename T_Callable>
    bool existsMember(ଅଚଳ T_Callable& p) ଅଚଳ {
        using T_ଗଣ୍ଠି = typename FunctionArgNoPointerNoCV<T_Callable, 1>::type;
        static_assert(
            vlstd::is_invocable_r<bool, T_Callable, ଅଚଳ AstClass*, ଅଚଳ T_ଗଣ୍ଠି*>::ମୂଲ୍ୟ
                && std::is_base_of<Ast_ଗଣ୍ଠି, T_ଗଣ୍ଠି>::ମୂଲ୍ୟ,
            "Predicate 'p' must have a signature compatible with 'bool(const AstClass*, "
            "const T_Node*)', with 'T_Node' being a subtype of 'AstNode'");
        ଯଦି (ଅଚଳ AstClassExtends* ଅଚଳ cextendsp = this->extendsp()) {
            ଯଦି (cextendsp->classp()->existsMember(p)) ଫେରନ୍ତୁ true;
        }
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (Ast_ଗଣ୍ଠି::privateTypeTest<T_ଗଣ୍ଠି>(stmtp)) {
                ଯଦି (p(this, static_cast<T_ଗଣ୍ଠି*>(stmtp))) ଫେରନ୍ତୁ true;
            }
        }
        ଫେରନ୍ତୁ false;
    }
};
class AstClassPackage final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Module {
    // The static information portion of a class (treated similarly to a package)
    //
    // @astgen ptr := m_classp : Optional[AstClass]  // Class package this is under
    //                                     // (weak pointer, hard link is other way)
ସର୍ବସାଧାରଣ:
    AstClassPackage(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : ASTGEN_SUPER_ClassPackage(fl, name, libname) {}
    ASTGEN_MEMBERS_AstClassPackage;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "classpackage"; }
    bool timescaleMatters() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    AstClass* classp() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_classp; }
    ଶୂନ୍ୟ classp(AstClass* classp) { m_classp = classp; }
};
class AstIface final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Module {
    // An interface declaration
    string m_dtypeNameFull;  // dtypeName(true) once frozen, as parameter types may then be gone
    string m_dtypeNameShort;  // dtypeName(false) once frozen, as parameter types may then be gone
    bool m_hasVirtualRef = false;  // There exists a virtual interface reference for this interface
ସର୍ବସାଧାରଣ:
    AstIface(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : ASTGEN_SUPER_Iface(fl, name, libname) {}
    ASTGEN_MEMBERS_AstIface;
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "interface"; }
    // Interfaces have `timescale applicability but lots of code seems to
    // get false warnings if we enable this
    bool timescaleMatters() ଅଚଳ override { ଫେରନ୍ତୁ false; }
    bool hasVirtualRef() ଅଚଳ { ଫେରନ୍ତୁ m_hasVirtualRef; }
    ଶୂନ୍ୟ setHasVirtualRef() { m_hasVirtualRef = true; }
    // Name of the interface as a data type, with the values of its parameters, e.g. 'ifc#(8)'
    string dtypeName(bool full) ଅଚଳ;
    // Fix dtypeName(), as V3WidthCommit moves parameter types to the type table
    ଶୂନ୍ୟ dtypeNameFreeze();
};
class AstModule final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Module {
    // A module declaration
    ଅଚଳ bool m_isChecker;  // Module represents a checker
    ଅଚଳ bool m_isProgram;  // Module represents a program
    bool m_hasGenericIface = false;  // Module contains a generic interface

ସର୍ବସାଧାରଣ:
    class Checker {};  // for constructor type-overload selection
    class Program {};  // for constructor type-overload selection
    AstModule(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{false} {}
    AstModule(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname, Checker)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{true}
        , m_isProgram{false} {}
    AstModule(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname, Program)
        : ASTGEN_SUPER_Module(fl, name, libname)
        , m_isChecker{false}
        , m_isProgram{true} {}
    ASTGEN_MEMBERS_AstModule;
    string verilogKwd() ଅଚଳ override {
        ଫେରନ୍ତୁ m_isChecker ? "checker" : m_isProgram ? "program" : "module";
    }
    bool timescaleMatters() ଅଚଳ override { ଫେରନ୍ତୁ true; }
    bool isChecker() ଅଚଳ { ଫେରନ୍ତୁ m_isChecker; }
    bool isProgram() ଅଚଳ { ଫେରନ୍ତୁ m_isProgram; }
    bool hasGenericIface() ଅଚଳ { ଫେରନ୍ତୁ m_hasGenericIface; }
    ଶୂନ୍ୟ hasGenericIface(bool hasGenericIface) { m_hasGenericIface = hasGenericIface; }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
};
class AstNotFoundModule final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Module {
    // A missing module declaration
ସର୍ବସାଧାରଣ:
    AstNotFoundModule(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : ASTGEN_SUPER_NotFoundModule(fl, name, libname) {}
    ASTGEN_MEMBERS_AstNotFoundModule;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "/*not-found-*/ module"; }
    bool timescaleMatters() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};
class AstPackage final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Module {
    // A package declaration
ସର୍ବସାଧାରଣ:
    AstPackage(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : ASTGEN_SUPER_Package(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPackage;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "package"; }
    bool timescaleMatters() ଅଚଳ override { ଫେରନ୍ତୁ !isDollarUnit() && !isConstPool(); }
};
class AstPrimitive final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Module {
    // A primitive declaration
ସର୍ବସାଧାରଣ:
    AstPrimitive(File_ଧାଡ଼ି* fl, ଅଚଳ string& name, ଅଚଳ string& libname)
        : ASTGEN_SUPER_Primitive(fl, name, libname) {}
    ASTGEN_MEMBERS_AstPrimitive;
    string verilogKwd() ଅଚଳ override { ଫେରନ୍ତୁ "primitive"; }
    bool timescaleMatters() ଅଚଳ override { ଫେରନ୍ତୁ false; }
};

// === AstNodeProcedure ===
class AstAlways final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list iff clocked
    ଅଚଳ VAlwaysKwd m_keyword;

ସର୍ବସାଧାରଣ:
    AstAlways(File_ଧାଡ଼ି* fl, VAlwaysKwd keyword, Ast_Sen_ବୃକ୍ଷ* sentreep, Ast_ଗଣ୍ଠି* stmtsp = nullptr)
        : ASTGEN_SUPER_Always(fl, stmtsp)
        , m_keyword{keyword} {
        this->sentreep(sentreep);
    }
    explicit ଧାଡ଼ିଭିତରେ AstAlways(AstAssignW* assignp);
    ASTGEN_MEMBERS_AstAlways;
    //
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    VAlwaysKwd keyword() ଅଚଳ { ଫେରନ୍ତୁ m_keyword; }
};
class AstAlwaysObserved final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // Like always but Observed scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

ସର୍ବସାଧାରଣ:
    AstAlwaysObserved(File_ଧାଡ଼ି* fl, Ast_Sen_ବୃକ୍ଷ* sentreep, Ast_ଗଣ୍ଠି* bodysp)
        : ASTGEN_SUPER_AlwaysObserved(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysObserved;
};
class AstAlwaysPost final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // Like always but 'post' scheduled, e.g. for array NBA commits

ସର୍ବସାଧାରଣ:
    explicit AstAlwaysPost(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_AlwaysPost(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPost;
};
class AstAlwaysPostponed final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // Like always but Postponed scheduling region

ସର୍ବସାଧାରଣ:
    AstAlwaysPostponed(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_AlwaysPostponed(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstAlwaysPostponed;
};
class AstAlwaysPre final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // Like always but 'pre' scheduled, e.g. for implementing NBAs

ସର୍ବସାଧାରଣ:
    explicit AstAlwaysPre(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_AlwaysPre(fl, nullptr) {}
    ASTGEN_MEMBERS_AstAlwaysPre;
};
class AstAlwaysReactive final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // Like always but Reactive scheduling region
    // @astgen op1 := sentreep : Optional[AstSenTree] // Sensitivity list, removed in V3Active

ସର୍ବସାଧାରଣ:
    AstAlwaysReactive(File_ଧାଡ଼ି* fl, Ast_Sen_ବୃକ୍ଷ* sentreep, Ast_ଗଣ୍ଠି* bodysp)
        : ASTGEN_SUPER_AlwaysReactive(fl, bodysp) {
        this->sentreep(sentreep);
    }
    ASTGEN_MEMBERS_AstAlwaysReactive;
};
class AstFinal final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
ସର୍ବସାଧାରଣ:
    AstFinal(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_Final(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstFinal;
};
class AstInitial final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
ସର୍ବସାଧାରଣ:
    AstInitial(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_Initial(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitial;
};
class AstInitialAutomatic final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // Automatic variable initialization
    // That is, it runs every function start, or class construction
ସର୍ବସାଧାରଣ:
    AstInitialAutomatic(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_InitialAutomatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialAutomatic;
};
class AstInitialStatic final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Procedure {
    // Static variable initialization
    // That is, it runs at the beginning of simulation, before 'initial' blocks
ସର୍ବସାଧାରଣ:
    AstInitialStatic(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* stmtsp)
        : ASTGEN_SUPER_InitialStatic(fl, stmtsp) {}
    ASTGEN_MEMBERS_AstInitialStatic;
};

// === AstNodeRange ===
class AstBracketRange final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Range {
    // Parser only concept C-style "[lhsp]", an AstUnknownRange, QueueRange or Range,
    // unknown until lhsp type is determined
    // @astgen op1 := elementsp : AstNode<AstNodeExpr|AstNodeDType>
ସର୍ବସାଧାରଣ:
    AstBracketRange(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି* elementsp)
        : ASTGEN_SUPER_BracketRange(fl) {
        this->elementsp(elementsp);
    }
    ASTGEN_MEMBERS_AstBracketRange;
    ଆଭାସୀ string emitC() { V3ERROR_NA_RETURN(""); }
    ଆଭାସୀ string emitVerilog() { V3ERROR_NA_RETURN(""); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
    // Will be removed in V3Width, which relies on this
    // being a child not a dtype pointed node
    bool maybePointedTo() ଅଚଳ override VL_MT_SAFE { ଫେରନ୍ତୁ false; }
};
class AstRange final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Range {
    // Range specification, for use under variables and cells
    // @astgen op1 := leftp : AstNodeExpr
    // @astgen op2 := rightp : AstNodeExpr
    ଅଚଳ bool m_fromBracket = false;  // From C-style '[x]' declaration
ସର୍ବସାଧାରଣ:
    AstRange(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* leftp, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* rightp, bool fromBracket = false)
        : ASTGEN_SUPER_Range(fl)
        , m_fromBracket{fromBracket} {
        this->leftp(leftp);
        this->rightp(rightp);
    }
    ଧାଡ଼ିଭିତରେ AstRange(File_ଧାଡ଼ି* fl, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ, ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ);
    ଧାଡ଼ିଭିତରେ AstRange(File_ଧାଡ଼ି* fl, ଅଚଳ VNumRange& range);
    ASTGEN_MEMBERS_AstRange;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ବାମ_Const() ଅଚଳ VL_MT_STABLE;
    ଧାଡ଼ିଭିତରେ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଡାହାଣ_Const() ଅଚଳ VL_MT_STABLE;
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା hiConst() ଅଚଳ VL_MT_STABLE {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା l = ବାମ_Const();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା r = ଡାହାଣ_Const();
        ଫେରନ୍ତୁ l > r ? l : r;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା loConst() ଅଚଳ VL_MT_STABLE {
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା l = ବାମ_Const();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା r = ଡାହାଣ_Const();
        ଫେରନ୍ତୁ l > r ? r : l;
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା elementsConst() ଅଚଳ VL_MT_STABLE { ଫେରନ୍ତୁ hiConst() - loConst() + 1; }
    bool ascending() ଅଚଳ { ଫେରନ୍ତୁ ବାମ_Const() < ଡାହାଣ_Const(); }
    // Iterate indices from left to right inclusive, see VNumRange::seqLeftToRight()
    VNumRange::Sequence seq_ବାମ_To_ଡାହାଣ() ଅଚଳ {
        ଫେରନ୍ତୁ VNumRange{ବାମ_Const(), ଡାହାଣ_Const()}.seq_ବାମ_To_ଡାହାଣ();
    }
    // Iterate indices from right to left inclusive, see VNumRange::seqRightToLeft()
    VNumRange::Sequence seq_ଡାହାଣ_To_ବାମ() ଅଚଳ {
        ଫେରନ୍ତୁ VNumRange{ବାମ_Const(), ଡାହାଣ_Const()}.seq_ଡାହାଣ_To_ବାମ();
    }
    ଶୂନ୍ୟ dump(std::ostream& str) ଅଚଳ override;
    ଶୂନ୍ୟ dumpJson(std::ostream& str) ଅଚଳ override;
    ଆଭାସୀ string emitC() { V3ERROR_NA_RETURN(""); }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* samep) ଅଚଳ override {
        ଅଚଳ AstRange* ଅଚଳ asamep = VN_DBG_AS(samep, Range);
        ଫେରନ୍ତୁ fromBracket() == asamep->fromBracket();
    }
    bool fromBracket() ଅଚଳ { ଫେରନ୍ତୁ m_fromBracket; }
};
class AstUnsizedRange final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Range {
    // Unsized range specification, for open arrays
ସର୍ବସାଧାରଣ:
    explicit AstUnsizedRange(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_UnsizedRange(fl) {}
    ASTGEN_MEMBERS_AstUnsizedRange;
    ଆଭାସୀ string emitC() { V3ERROR_NA_RETURN(""); }
    ଆଭାସୀ string emitVerilog() { ଫେରନ୍ତୁ "[]"; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};
class AstWildcardRange final : ସର୍ବସାଧାରଣ Ast_ଗଣ୍ଠି_Range {
    // Wildcard range specification, for wildcard index type associative arrays
ସର୍ବସାଧାରଣ:
    explicit AstWildcardRange(File_ଧାଡ଼ି* fl)
        : ASTGEN_SUPER_WildcardRange(fl) {}
    ASTGEN_MEMBERS_AstWildcardRange;
    ଆଭାସୀ string emitC() { V3ERROR_NA_RETURN(""); }
    ଆଭାସୀ string emitVerilog() { ଫେରନ୍ତୁ "[*]"; }
    bool same_ଗଣ୍ଠି(ଅଚଳ Ast_ଗଣ୍ଠି* /*samep*/) ଅଚଳ override { ଫେରନ୍ତୁ true; }
};

#endif  // Guard
