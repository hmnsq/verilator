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

#اگر_غیر_تعریف_شدہ VERILATOR_V3EMITCBASE_H_
#تعریف_کرو VERILATOR_V3EMITCBASE_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3File.h"

#شامل <cmath>
#شامل <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ثابت VNUser4_اندر_Use user4_اندر_Use;

عوامی:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ساکن ثابت Ast_گرہ_Module* get(ثابت Ast_گرہ* nodep) VL_MT_STABLE {
        واپس VN_AS(nodep->user4p(), گرہ_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
عوامی:
    ساکن string void_Self_تفویض(ثابت Ast_گرہ_Module* modp) {
        ثابت string className = سابقہ_Name_Protect(modp);
        واپس className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ساکن string pchClassName() VL_MT_STABLE { واپس v3Global.opt.سابقہ() + "__pch"; }
    ساکن string symClassName() VL_MT_STABLE {
        واپس v3Global.opt.سابقہ() + "_" + VIdProtect::protect("_Syms");
    }
    ساکن string symClassVar() { واپس symClassName() + "* __restrict vlSymsp"; }
    ساکن string sym_Class_تفویض() {
        واپس symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ساکن string چوٹی_Class_Name() VL_MT_محفوظ {  // Return name of top wrapper module
        واپس v3Global.opt.سابقہ();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ساکن string exec_گراف_Name(ثابت Ast_Exec_گراف* nodep) {
        واپس چوٹی_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ساکن std::string constPoolName(ثابت AstVar* varp) {
        UASSERT_OBJ(varp->const_Pool_اندراج(), varp, "Not a constant pool variable");
        واپس چوٹی_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ساکن string سابقہ_Name_Protect(ثابت Ast_گرہ* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : عوامی VNVisitorConst {
عوامی:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class متن_Stmts final {
    عوامی:
        ساخت Stmt final {
            std::string m_متن;  // Statement text
            سائز_t m_لاگت;  // Cost of statement, for splitting
        };

    نجی:
        std::سمتیہ<Stmt> m_stmts;  // Statements

    عوامی:
        // Add a statement
        خالی جوڑو(ثابت std::string& stmt) {
            ثابت سائز_t لاگت = stmt.خالی_خانے() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_گرہ::INSTR_گنتی_CTOR_DTOR;
            m_stmts.ڈالو_عقب({stmt, لاگت});
        }
        سائز_t سائز() ثابت { واپس m_stmts.سائز(); }
        ثابت Stmt& عامل[](سائز_t i) ثابت { واپس m_stmts[i]; }
        std::سمتیہ<Stmt>::const_iterator begin() ثابت { واپس m_stmts.begin(); }
        std::سمتیہ<Stmt>::const_iterator انتہا() ثابت { واپس m_stmts.انتہا(); }
    };

نجی:
    // STATE
    V3_برآمد_CFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::سمتیہ<AstCFile*> m_newCfileps;  // AstCFiles created
    سائز_t m_بانٹو_سائز = 0;  // Complexity of this file
    ثابت سائز_t m_بانٹو_حد_بندی = v3Global.opt.output_بانٹو()
                                    ? static_cast<سائز_t>(v3Global.opt.output_بانٹو())
                                    : std::numeric_limits<سائز_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    خالی openNewOutputFile(ثابت std::string& بنیاد_Name, bool slow, bool support, bool ماخذ,
                           ثابت حرف* ثابت descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + بنیاد_Name;
        اگر (ماخذ) {
            اگر (slow) fileName += "__Slow";
            fileName += ".cpp";
        } ورنہ {
            fileName += ".h";
        }
        AstCFile* ثابت cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->ماخذ(ماخذ);
        cfilep->support(support);
        m_newCfileps.emplace_عقب(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emit_بانٹو_متن_Stmts(متن_Stmts& stmts, ثابت std::string& سابقہ,
                                   ثابت std::string& params, ثابت std::string& دلائل,
                                   سائز_t max_لاگت, bool support, ثابت حرف* descriptionp,
                                   ثابت std::function<خالی()>& emit_سرخی);

عوامی:
    // Create new source AstCFile and open it for writing
    خالی open_New_Output_ماخذ_File(ثابت std::string& بنیاد_Name, bool slow, bool support,
                                 ثابت حرف* descriptionp) {
        V3Stats::جوڑو_Stat_مجموع(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(بنیاد_Name, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    خالی open_New_Output_سرخی_File(ثابت std::string& بنیاد_Name, ثابت حرف* descriptionp) {
        openNewOutputFile(بنیاد_Name, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    خالی openOutputFile(AstCFile* cfilep, ثابت حرف* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_بانٹو_سائز = 0;
        اگر (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3_برآمد_CFile{VL_DEV_NULL};
            واپس;
        }
        m_ofp = new V3_برآمد_CFile{cfilep->name()};
        puts_سرخی();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    خالی closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_بانٹو_سائز);
        m_cfilep = nullptr;
    }

    std::سمتیہ<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        واپس std::move(m_newCfileps);
    }

    خالی بانٹو_سائز_Inc(سائز_t گنتی) { m_بانٹو_سائز += گنتی; }
    خالی بانٹو_سائز_Inc(ثابت Ast_گرہ* nodep) {
        بانٹو_سائز_Inc(static_cast<سائز_t>(nodep->گرہ_گنتی()));
    }
    bool بانٹو_Needed(سائز_t بانٹو_حد_بندی) ثابت { واپس m_بانٹو_سائز >= بانٹو_حد_بندی; }
    bool بانٹو_Needed() ثابت { واپس بانٹو_Needed(m_بانٹو_حد_بندی); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emit_بانٹو_Ctor_Dtor(متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, سائز_t بنیاد_لاگت,
                      ثابت std::string& سابقہ, ثابت std::string& params,
                      ثابت std::string& دلائل, bool support, ثابت حرف* descriptionp,
                      ثابت std::function<خالی()>& emit_سرخی);

    // Returns pointer to current output file object.
    V3_برآمد_CFile* ofp() ثابت VL_MT_محفوظ { واپس m_ofp; }

    خالی puts(ثابت string& str) { ofp()->puts(str); }
    خالی putns(ثابت Ast_گرہ* nodep, ثابت string& str) { ofp()->putns(nodep, str); }
    خالی puts_سرخی() { ofp()->puts_سرخی(); }
    خالی putbs(ثابت string& str) { ofp()->putbs(str); }
    خالی putnbs(ثابت Ast_گرہ* nodep, ثابت string& str) { ofp()->putnbs(nodep, str); }
    خالی putsDecoration(ثابت Ast_گرہ* nodep, ثابت string& str) {
        اگر (v3Global.opt.decoration()) putns(nodep, str);
    }
    خالی putsQuoted(ثابت string& str) { ofp()->putsQuoted(str); }
    خالی ensure_New_سطر() { ofp()->ensure_New_سطر(); }
    bool optSystemC() { واپس v3Global.opt.systemC(); }
    ساکن string protect(ثابت string& name) VL_MT_محفوظ { واپس VIdProtect::protect(name); }
    ساکن string funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_گرہ_Module* modp = nullptr);
    ساکن bool isExplicitlyConstructed(ثابت AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ثابت Ast_گرہ_Module* ثابت modp = EmitCParentModule::get(varp);
        اگر (!modp || !varp->isModelState()) واپس false;
        // Classes use real constructors
        اگر (VN_IS(modp, Class)) واپس false;
        // Constant pool has special handling
        اگر (modp->isConstPool()) واپس false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        واپس !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    ساکن std::string varName(ثابت AstVar* varp) VL_MT_STABLE {
        ثابت std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        اگر (varp->isPrimaryIO()) واپس "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        اگر (isExplicitlyConstructed(varp)) واپس "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        واپس name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string c_Func_دلائل(ثابت AstCFunc* nodep, bool cpp = true);
    خالی emitCDefaultConstructor(ثابت Ast_گرہ_Module* modp);
    خالی emit_CFunc_سرخی(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp, bool withScope);
    خالی emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp, bool cLinkage = false);
    خالی emitVarDecl(ثابت AstVar* nodep, bool asRef = false);
    خالی emitVarAccessors(ثابت AstVar* nodep);
    سانچہ <typename T_Callable>
    ساکن خالی forModCUse(ثابت Ast_گرہ_Module* modp, VUseType useType, T_Callable action) {
        برائے (ثابت Ast_گرہ* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            اگر (ثابت AstCUse* ثابت usep = VN_CAST(itemp, CUse)) {
                اگر (usep->useType().شامل_ہے_Any(useType)) {
                    اگر (usep->useType().شامل_ہے_Any(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::سابقہ_Name_Protect(usep) + ".h\"\n");
                        جاری;  // Forward declaration is not necessary
                    }
                    اگر (usep->useType().شامل_ہے_Any(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::سابقہ_Name_Protect(usep) + ";\n");
                    }
                }
            }
        }
    }
    خالی emitModCUse(ثابت Ast_گرہ_Module* modp, VUseType useType);
    خالی emitSystemCSection(ثابت Ast_گرہ_Module* modp, VSystemCSectionType type);
    ساکن std::pair<string, File_سطر*> scSection(ثابت Ast_گرہ_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = طےشدہ;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        برائے (AstCFile* ثابت cfilep : m_newCfileps) v3Global.rootp()->جوڑو_Filesp(cfilep);
    }
};

#اگر_ختم  // guard
