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

#جے_بنا_تعریف VERILATOR_V3EMITCBASE_H_
#تعریف_کرو VERILATOR_V3EMITCBASE_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3File.h"

#ضمن <cmath>
#ضمن <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ثابت VNUser4_وچ_Use user4_وچ_Use;

عام:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ساكن ثابت Ast_گرہ_Module* get(ثابت Ast_گرہ* nodep) VL_MT_STABLE {
        أرجع VN_AS(nodep->user4p(), گرہ_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
عام:
    ساكن string void_Self_سونپو(ثابت Ast_گرہ_Module* modp) {
        ثابت string className = اگیتر_Name_Protect(modp);
        أرجع className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ساكن string pchClassName() VL_MT_STABLE { أرجع v3Global.opt.اگیتر() + "__pch"; }
    ساكن string symClassName() VL_MT_STABLE {
        أرجع v3Global.opt.اگیتر() + "_" + VIdProtect::protect("_Syms");
    }
    ساكن string symClassVar() { أرجع symClassName() + "* __restrict vlSymsp"; }
    ساكن string sym_Class_سونپو() {
        أرجع symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ساكن string چوٹی_Class_Name() VL_MT_محفوظ {  // Return name of top wrapper module
        أرجع v3Global.opt.اگیتر();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ساكن string exec_گراف_Name(ثابت Ast_Exec_گراف* nodep) {
        أرجع چوٹی_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ساكن std::string constPoolName(ثابت AstVar* varp) {
        UASSERT_OBJ(varp->const_Pool_اندراج(), varp, "Not a constant pool variable");
        أرجع چوٹی_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ساكن string اگیتر_Name_Protect(ثابت Ast_گرہ* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : عام VNVisitorConst {
عام:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class متن_Stmts final {
    عام:
        بنية Stmt final {
            std::string m_متن;  // Statement text
            ناپ_t m_لاگت;  // Cost of statement, for splitting
        };

    خاص:
        std::سمتیہ<Stmt> m_stmts;  // Statements

    عام:
        // Add a statement
        فراغ جوڑو(ثابت std::string& stmt) {
            ثابت ناپ_t لاگت = stmt.خالی_تھاواں() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_گرہ::INSTR_گنتی_CTOR_DTOR;
            m_stmts.ڈالو_پٹھ({stmt, لاگت});
        }
        ناپ_t ناپ() ثابت { أرجع m_stmts.ناپ(); }
        ثابت Stmt& عامل[](ناپ_t i) ثابت { أرجع m_stmts[i]; }
        std::سمتیہ<Stmt>::const_iterator begin() ثابت { أرجع m_stmts.begin(); }
        std::سمتیہ<Stmt>::const_iterator سرا() ثابت { أرجع m_stmts.سرا(); }
    };

خاص:
    // STATE
    V3_باہرلا_CFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::سمتیہ<AstCFile*> m_newCfileps;  // AstCFiles created
    ناپ_t m_ونڈو_ناپ = 0;  // Complexity of this file
    ثابت ناپ_t m_ونڈو_حد_بندی = v3Global.opt.output_ونڈو()
                                    ? static_cast<ناپ_t>(v3Global.opt.output_ونڈو())
                                    : std::numeric_limits<ناپ_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    فراغ openNewOutputFile(ثابت std::string& ادھار_Name, bool slow, bool support, bool سروت,
                           ثابت حرف* ثابت descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + ادھار_Name;
        إذا (سروت) {
            إذا (slow) fileName += "__Slow";
            fileName += ".cpp";
        } وإلا {
            fileName += ".h";
        }
        AstCFile* ثابت cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->سروت(سروت);
        cfilep->support(support);
        m_newCfileps.emplace_پٹھ(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emit_ونڈو_متن_Stmts(متن_Stmts& stmts, ثابت std::string& اگیتر,
                                   ثابت std::string& params, ثابت std::string& دلیلاں,
                                   ناپ_t max_لاگت, bool support, ثابت حرف* descriptionp,
                                   ثابت std::function<فراغ()>& emit_سرخی);

عام:
    // Create new source AstCFile and open it for writing
    فراغ open_New_Output_سروت_File(ثابت std::string& ادھار_Name, bool slow, bool support,
                                 ثابت حرف* descriptionp) {
        V3Stats::جوڑو_Stat_جوڑ(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(ادھار_Name, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    فراغ open_New_Output_سرخی_File(ثابت std::string& ادھار_Name, ثابت حرف* descriptionp) {
        openNewOutputFile(ادھار_Name, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    فراغ openOutputFile(AstCFile* cfilep, ثابت حرف* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_ونڈو_ناپ = 0;
        إذا (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3_باہرلا_CFile{VL_DEV_NULL};
            أرجع;
        }
        m_ofp = new V3_باہرلا_CFile{cfilep->name()};
        puts_سرخی();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    فراغ closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_ونڈو_ناپ);
        m_cfilep = nullptr;
    }

    std::سمتیہ<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        أرجع std::move(m_newCfileps);
    }

    فراغ ونڈو_ناپ_Inc(ناپ_t گنتی) { m_ونڈو_ناپ += گنتی; }
    فراغ ونڈو_ناپ_Inc(ثابت Ast_گرہ* nodep) {
        ونڈو_ناپ_Inc(static_cast<ناپ_t>(nodep->گرہ_گنتی()));
    }
    bool ونڈو_Needed(ناپ_t ونڈو_حد_بندی) ثابت { أرجع m_ونڈو_ناپ >= ونڈو_حد_بندی; }
    bool ونڈو_Needed() ثابت { أرجع ونڈو_Needed(m_ونڈو_حد_بندی); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emit_ونڈو_Ctor_Dtor(متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, ناپ_t ادھار_لاگت,
                      ثابت std::string& اگیتر, ثابت std::string& params,
                      ثابت std::string& دلیلاں, bool support, ثابت حرف* descriptionp,
                      ثابت std::function<فراغ()>& emit_سرخی);

    // Returns pointer to current output file object.
    V3_باہرلا_CFile* ofp() ثابت VL_MT_محفوظ { أرجع m_ofp; }

    فراغ puts(ثابت string& str) { ofp()->puts(str); }
    فراغ putns(ثابت Ast_گرہ* nodep, ثابت string& str) { ofp()->putns(nodep, str); }
    فراغ puts_سرخی() { ofp()->puts_سرخی(); }
    فراغ putbs(ثابت string& str) { ofp()->putbs(str); }
    فراغ putnbs(ثابت Ast_گرہ* nodep, ثابت string& str) { ofp()->putnbs(nodep, str); }
    فراغ putsDecoration(ثابت Ast_گرہ* nodep, ثابت string& str) {
        إذا (v3Global.opt.decoration()) putns(nodep, str);
    }
    فراغ putsQuoted(ثابت string& str) { ofp()->putsQuoted(str); }
    فراغ ensure_New_سطر() { ofp()->ensure_New_سطر(); }
    bool optSystemC() { أرجع v3Global.opt.systemC(); }
    ساكن string protect(ثابت string& name) VL_MT_محفوظ { أرجع VIdProtect::protect(name); }
    ساكن string funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_گرہ_Module* modp = nullptr);
    ساكن bool isExplicitlyConstructed(ثابت AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ثابت Ast_گرہ_Module* ثابت modp = EmitCParentModule::get(varp);
        إذا (!modp || !varp->isModelState()) أرجع false;
        // Classes use real constructors
        إذا (VN_IS(modp, Class)) أرجع false;
        // Constant pool has special handling
        إذا (modp->isConstPool()) أرجع false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        أرجع !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    ساكن std::string varName(ثابت AstVar* varp) VL_MT_STABLE {
        ثابت std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        إذا (varp->isPrimaryIO()) أرجع "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        إذا (isExplicitlyConstructed(varp)) أرجع "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        أرجع name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string c_Func_دلیلاں(ثابت AstCFunc* nodep, bool cpp = true);
    فراغ emitCDefaultConstructor(ثابت Ast_گرہ_Module* modp);
    فراغ emit_CFunc_سرخی(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp, bool withScope);
    فراغ emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_گرہ_Module* modp, bool cLinkage = false);
    فراغ emitVarDecl(ثابت AstVar* nodep, bool asRef = false);
    فراغ emitVarAccessors(ثابت AstVar* nodep);
    سانچا <typename T_Callable>
    ساكن فراغ forModCUse(ثابت Ast_گرہ_Module* modp, VUseType useType, T_Callable action) {
        لكل (ثابت Ast_گرہ* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            إذا (ثابت AstCUse* ثابت usep = VN_CAST(itemp, CUse)) {
                إذا (usep->useType().شامل_اے_Any(useType)) {
                    إذا (usep->useType().شامل_اے_Any(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::اگیتر_Name_Protect(usep) + ".h\"\n");
                        استمر;  // Forward declaration is not necessary
                    }
                    إذا (usep->useType().شامل_اے_Any(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::اگیتر_Name_Protect(usep) + ";\n");
                    }
                }
            }
        }
    }
    فراغ emitModCUse(ثابت Ast_گرہ_Module* modp, VUseType useType);
    فراغ emitSystemCSection(ثابت Ast_گرہ_Module* modp, VSystemCSectionType type);
    ساكن std::pair<string, File_سطر*> scSection(ثابت Ast_گرہ_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = افتراضي;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        لكل (AstCFile* ثابت cfilep : m_newCfileps) v3Global.rootp()->جوڑو_Filesp(cfilep);
    }
};

#جے_انت  // guard
