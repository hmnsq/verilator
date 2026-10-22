// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for tree
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3EMITCBASE_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3EMITCBASE_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3File.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <cmath>
#ਅਨ੍ਤਰ੍ਭਾਵ <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ਅਚਲ VNUser4_ਵਿੱਚ_Use user4_ਵਿੱਚ_Use;

ਜਨਤਕ:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ਸਥਿਰ ਅਚਲ Ast_ਗੰਢ_Module* get(ਅਚਲ Ast_ਗੰਢ* nodep) VL_MT_STABLE {
        ਵਾਪਸ VN_AS(nodep->user4p(), ਗੰਢ_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
ਜਨਤਕ:
    ਸਥਿਰ string void_Self_ਸੌਂਪੋ(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਅਚਲ string className = ਅਗੇਤਰ_Name_Protect(modp);
        ਵਾਪਸ className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ਸਥਿਰ string pchClassName() VL_MT_STABLE { ਵਾਪਸ v3Global.opt.ਅਗੇਤਰ() + "__pch"; }
    ਸਥਿਰ string symClassName() VL_MT_STABLE {
        ਵਾਪਸ v3Global.opt.ਅਗੇਤਰ() + "_" + VIdProtect::protect("_Syms");
    }
    ਸਥਿਰ string symClassVar() { ਵਾਪਸ symClassName() + "* __restrict vlSymsp"; }
    ਸਥਿਰ string sym_Class_ਸੌਂਪੋ() {
        ਵਾਪਸ symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ਸਥਿਰ string ਸਿਖਰ_Class_Name() VL_MT_ਸੁਰੱਖਿਅਤ {  // Return name of top wrapper module
        ਵਾਪਸ v3Global.opt.ਅਗੇਤਰ();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ਸਥਿਰ string exec_ਗ੍ਰਾਫ_Name(ਅਚਲ Ast_Exec_ਗ੍ਰਾਫ* nodep) {
        ਵਾਪਸ ਸਿਖਰ_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ਸਥਿਰ std::string constPoolName(ਅਚਲ AstVar* varp) {
        UASSERT_OBJ(varp->const_Pool_ਇੰਦਰਾਜ(), varp, "Not a constant pool variable");
        ਵਾਪਸ ਸਿਖਰ_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ਸਥਿਰ string ਅਗੇਤਰ_Name_Protect(ਅਚਲ Ast_ਗੰਢ* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : ਜਨਤਕ VNVisitorConst {
ਜਨਤਕ:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class ਪਾਠ_Stmts final {
    ਜਨਤਕ:
        ਬਣਤਰ Stmt final {
            std::string m_ਪਾਠ;  // Statement text
            ਮਾਪ_t m_ਲਾਗਤ;  // Cost of statement, for splitting
        };

    ਨਿਜੀ:
        std::ਸਦਿਸ਼<Stmt> m_stmts;  // Statements

    ਜਨਤਕ:
        // Add a statement
        ਖਾਲੀ ਜੋੜੋ(ਅਚਲ std::string& stmt) {
            ਅਚਲ ਮਾਪ_t ਲਾਗਤ = stmt.ਖਾਲੀ_ਥਾਵਾਂ() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_ਗੰਢ::INSTR_ਗਿਣਤੀ_CTOR_DTOR;
            m_stmts.ਧੱਕੋ_ਪਿੱਠ({stmt, ਲਾਗਤ});
        }
        ਮਾਪ_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_stmts.ਮਾਪ(); }
        ਅਚਲ Stmt& ਸੰਚਾਲਕ[](ਮਾਪ_t i) ਅਚਲ { ਵਾਪਸ m_stmts[i]; }
        std::ਸਦਿਸ਼<Stmt>::const_iterator begin() ਅਚਲ { ਵਾਪਸ m_stmts.begin(); }
        std::ਸਦਿਸ਼<Stmt>::const_iterator ਸਿਰਾ() ਅਚਲ { ਵਾਪਸ m_stmts.ਸਿਰਾ(); }
    };

ਨਿਜੀ:
    // STATE
    V3_ਬਾਹਰਲਾ_CFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::ਸਦਿਸ਼<AstCFile*> m_newCfileps;  // AstCFiles created
    ਮਾਪ_t m_ਵੰਡੋ_ਮਾਪ = 0;  // Complexity of this file
    ਅਚਲ ਮਾਪ_t m_ਵੰਡੋ_ਹੱਦਬੰਦੀ = v3Global.opt.output_ਵੰਡੋ()
                                    ? static_cast<ਮਾਪ_t>(v3Global.opt.output_ਵੰਡੋ())
                                    : std::numeric_limits<ਮਾਪ_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    ਖਾਲੀ openNewOutputFile(ਅਚਲ std::string& ਅਧਾਰ_Name, bool slow, bool support, bool ਸਰੋਤ,
                           ਅਚਲ ਅੱਖਰ* ਅਚਲ descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + ਅਧਾਰ_Name;
        ਜੇ (ਸਰੋਤ) {
            ਜੇ (slow) fileName += "__Slow";
            fileName += ".cpp";
        } ਨਹੀਂਤਾਂ {
            fileName += ".h";
        }
        AstCFile* ਅਚਲ cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->ਸਰੋਤ(ਸਰੋਤ);
        cfilep->support(support);
        m_newCfileps.emplace_ਪਿੱਠ(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emit_ਵੰਡੋ_ਪਾਠ_Stmts(ਪਾਠ_Stmts& stmts, ਅਚਲ std::string& ਅਗੇਤਰ,
                                   ਅਚਲ std::string& params, ਅਚਲ std::string& ਦਲੀਲਾਂ,
                                   ਮਾਪ_t max_ਲਾਗਤ, bool support, ਅਚਲ ਅੱਖਰ* descriptionp,
                                   ਅਚਲ std::function<ਖਾਲੀ()>& emit_ਸਿਰਲੇਖ);

ਜਨਤਕ:
    // Create new source AstCFile and open it for writing
    ਖਾਲੀ open_New_Output_ਸਰੋਤ_File(ਅਚਲ std::string& ਅਧਾਰ_Name, bool slow, bool support,
                                 ਅਚਲ ਅੱਖਰ* descriptionp) {
        V3Stats::ਜੋੜੋ_Stat_ਜੋੜ(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(ਅਧਾਰ_Name, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    ਖਾਲੀ open_New_Output_ਸਿਰਲੇਖ_File(ਅਚਲ std::string& ਅਧਾਰ_Name, ਅਚਲ ਅੱਖਰ* descriptionp) {
        openNewOutputFile(ਅਧਾਰ_Name, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    ਖਾਲੀ openOutputFile(AstCFile* cfilep, ਅਚਲ ਅੱਖਰ* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_ਵੰਡੋ_ਮਾਪ = 0;
        ਜੇ (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3_ਬਾਹਰਲਾ_CFile{VL_DEV_NULL};
            ਵਾਪਸ;
        }
        m_ofp = new V3_ਬਾਹਰਲਾ_CFile{cfilep->name()};
        puts_ਸਿਰਲੇਖ();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    ਖਾਲੀ closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_ਵੰਡੋ_ਮਾਪ);
        m_cfilep = nullptr;
    }

    std::ਸਦਿਸ਼<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        ਵਾਪਸ std::move(m_newCfileps);
    }

    ਖਾਲੀ ਵੰਡੋ_ਮਾਪ_Inc(ਮਾਪ_t ਗਿਣਤੀ_2) { m_ਵੰਡੋ_ਮਾਪ += ਗਿਣਤੀ_2; }
    ਖਾਲੀ ਵੰਡੋ_ਮਾਪ_Inc(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਵੰਡੋ_ਮਾਪ_Inc(static_cast<ਮਾਪ_t>(nodep->ਗੰਢ_ਗਿਣਤੀ()));
    }
    bool ਵੰਡੋ_Needed(ਮਾਪ_t ਵੰਡੋ_ਹੱਦਬੰਦੀ) ਅਚਲ { ਵਾਪਸ m_ਵੰਡੋ_ਮਾਪ >= ਵੰਡੋ_ਹੱਦਬੰਦੀ; }
    bool ਵੰਡੋ_Needed() ਅਚਲ { ਵਾਪਸ ਵੰਡੋ_Needed(m_ਵੰਡੋ_ਹੱਦਬੰਦੀ); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emit_ਵੰਡੋ_Ctor_Dtor(ਪਾਠ_Stmts& ctorStmts, ਪਾਠ_Stmts& dtorStmts, ਮਾਪ_t ਅਧਾਰ_ਲਾਗਤ,
                      ਅਚਲ std::string& ਅਗੇਤਰ, ਅਚਲ std::string& params,
                      ਅਚਲ std::string& ਦਲੀਲਾਂ, bool support, ਅਚਲ ਅੱਖਰ* descriptionp,
                      ਅਚਲ std::function<ਖਾਲੀ()>& emit_ਸਿਰਲੇਖ);

    // Returns pointer to current output file object.
    V3_ਬਾਹਰਲਾ_CFile* ofp() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ m_ofp; }

    ਖਾਲੀ puts(ਅਚਲ string& str) { ofp()->puts(str); }
    ਖਾਲੀ putns(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& str) { ofp()->putns(nodep, str); }
    ਖਾਲੀ puts_ਸਿਰਲੇਖ() { ofp()->puts_ਸਿਰਲੇਖ(); }
    ਖਾਲੀ putbs(ਅਚਲ string& str) { ofp()->putbs(str); }
    ਖਾਲੀ putnbs(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& str) { ofp()->putnbs(nodep, str); }
    ਖਾਲੀ putsDecoration(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ string& str) {
        ਜੇ (v3Global.opt.decoration()) putns(nodep, str);
    }
    ਖਾਲੀ putsQuoted(ਅਚਲ string& str) { ofp()->putsQuoted(str); }
    ਖਾਲੀ ensure_New_ਸਤਰ() { ofp()->ensure_New_ਸਤਰ(); }
    bool optSystemC() { ਵਾਪਸ v3Global.opt.systemC(); }
    ਸਥਿਰ string protect(ਅਚਲ string& name) VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ VIdProtect::protect(name); }
    ਸਥਿਰ string funcNameProtect(ਅਚਲ AstCFunc* nodep, ਅਚਲ Ast_ਗੰਢ_Module* modp = nullptr);
    ਸਥਿਰ bool isExplicitlyConstructed(ਅਚਲ AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ modp = EmitCParentModule::get(varp);
        ਜੇ (!modp || !varp->isModelState()) ਵਾਪਸ false;
        // Classes use real constructors
        ਜੇ (VN_IS(modp, Class)) ਵਾਪਸ false;
        // Constant pool has special handling
        ਜੇ (modp->isConstPool()) ਵਾਪਸ false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        ਵਾਪਸ !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    ਸਥਿਰ std::string varName(ਅਚਲ AstVar* varp) VL_MT_STABLE {
        ਅਚਲ std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        ਜੇ (varp->isPrimaryIO()) ਵਾਪਸ "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        ਜੇ (isExplicitlyConstructed(varp)) ਵਾਪਸ "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        ਵਾਪਸ name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string c_Func_ਦਲੀਲਾਂ(ਅਚਲ AstCFunc* nodep, bool cpp = true);
    ਖਾਲੀ emitCDefaultConstructor(ਅਚਲ Ast_ਗੰਢ_Module* modp);
    ਖਾਲੀ emit_CFunc_ਸਿਰਲੇਖ(ਅਚਲ AstCFunc* funcp, ਅਚਲ Ast_ਗੰਢ_Module* modp, bool withScope);
    ਖਾਲੀ emitCFuncDecl(ਅਚਲ AstCFunc* funcp, ਅਚਲ Ast_ਗੰਢ_Module* modp, bool cLinkage = false);
    ਖਾਲੀ emitVarDecl(ਅਚਲ AstVar* nodep, bool asRef = false);
    ਖਾਲੀ emitVarAccessors(ਅਚਲ AstVar* nodep);
    ਸਾਂਚਾ <typename T_Callable>
    ਸਥਿਰ ਖਾਲੀ forModCUse(ਅਚਲ Ast_ਗੰਢ_Module* modp, VUseType useType, T_Callable action) {
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (ਅਚਲ AstCUse* ਅਚਲ usep = VN_CAST(itemp, CUse)) {
                ਜੇ (usep->useType().ਸ਼ਾਮਲ_ਹੈ_Any(useType)) {
                    ਜੇ (usep->useType().ਸ਼ਾਮਲ_ਹੈ_Any(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::ਅਗੇਤਰ_Name_Protect(usep) + ".h\"\n");
                        ਜਾਰੀ;  // Forward declaration is not necessary
                    }
                    ਜੇ (usep->useType().ਸ਼ਾਮਲ_ਹੈ_Any(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::ਅਗੇਤਰ_Name_Protect(usep) + ";\n");
                    }
                }
            }
        }
    }
    ਖਾਲੀ emitModCUse(ਅਚਲ Ast_ਗੰਢ_Module* modp, VUseType useType);
    ਖਾਲੀ emitSystemCSection(ਅਚਲ Ast_ਗੰਢ_Module* modp, VSystemCSectionType type);
    ਸਥਿਰ std::pair<string, File_ਸਤਰ*> scSection(ਅਚਲ Ast_ਗੰਢ_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = ਮੂਲ;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        ਲਈ (AstCFile* ਅਚਲ cfilep : m_newCfileps) v3Global.rootp()->ਜੋੜੋ_Filesp(cfilep);
    }
};

#ਜੇ_ਅੰਤ  // guard
