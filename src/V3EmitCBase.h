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

#اگر_تعریف_نشده VERILATOR_V3EMITCBASE_H_
#تعریف_کن VERILATOR_V3EMITCBASE_H_

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
    ثابت VNUser4_درون_Use user4_درون_Use;

عام:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ساكن ثابت Ast_گره_Module* get(ثابت Ast_گره* nodep) VL_MT_STABLE {
        أرجع VN_AS(nodep->user4p(), گره_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
عام:
    ساكن string void_Self_تخصیص_بده(ثابت Ast_گره_Module* modp) {
        ثابت string className = پیشوند_Name_Protect(modp);
        أرجع className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ساكن string pchClassName() VL_MT_STABLE { أرجع v3Global.opt.پیشوند() + "__pch"; }
    ساكن string symClassName() VL_MT_STABLE {
        أرجع v3Global.opt.پیشوند() + "_" + VIdProtect::protect("_Syms");
    }
    ساكن string symClassVar() { أرجع symClassName() + "* __restrict vlSymsp"; }
    ساكن string sym_Class_تخصیص_بده() {
        أرجع symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ساكن string بالاترین_Class_Name() VL_MT_امن {  // Return name of top wrapper module
        أرجع v3Global.opt.پیشوند();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ساكن string exec_گراف_Name(ثابت Ast_Exec_گراف* nodep) {
        أرجع بالاترین_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ساكن std::string constPoolName(ثابت AstVar* varp) {
        UASSERT_OBJ(varp->const_Pool_مدخل(), varp, "Not a constant pool variable");
        أرجع بالاترین_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ساكن string پیشوند_Name_Protect(ثابت Ast_گره* nodep) VL_MT_STABLE;
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
            اندازه_t m_هزینه;  // Cost of statement, for splitting
        };

    خاص:
        std::وکتور<Stmt> m_stmts;  // Statements

    عام:
        // Add a statement
        فراغ جمع_کن(ثابت std::string& stmt) {
            ثابت اندازه_t هزینه = stmt.خالی() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_گره::INSTR_شمار_CTOR_DTOR;
            m_stmts.افزودن_پشت({stmt, هزینه});
        }
        اندازه_t اندازه() ثابت { أرجع m_stmts.اندازه(); }
        ثابت Stmt& عملگر[](اندازه_t i) ثابت { أرجع m_stmts[i]; }
        std::وکتور<Stmt>::const_iterator begin() ثابت { أرجع m_stmts.begin(); }
        std::وکتور<Stmt>::const_iterator انتها() ثابت { أرجع m_stmts.انتها(); }
    };

خاص:
    // STATE
    V3_بیرون_CFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::وکتور<AstCFile*> m_newCfileps;  // AstCFiles created
    اندازه_t m_تقسیم_اندازه = 0;  // Complexity of this file
    ثابت اندازه_t m_تقسیم_حد = v3Global.opt.output_تقسیم()
                                    ? static_cast<اندازه_t>(v3Global.opt.output_تقسیم())
                                    : std::numeric_limits<اندازه_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    فراغ openNewOutputFile(ثابت std::string& پایه_Name, bool slow, bool support, bool منبع,
                           ثابت حرف* ثابت descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + پایه_Name;
        إذا (منبع) {
            إذا (slow) fileName += "__Slow";
            fileName += ".cpp";
        } وإلا {
            fileName += ".h";
        }
        AstCFile* ثابت cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->منبع(منبع);
        cfilep->support(support);
        m_newCfileps.emplace_پشت(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emit_تقسیم_متن_Stmts(متن_Stmts& stmts, ثابت std::string& پیشوند,
                                   ثابت std::string& params, ثابت std::string& آرگومانها,
                                   اندازه_t max_هزینه, bool support, ثابت حرف* descriptionp,
                                   ثابت std::function<فراغ()>& emit_سرآیند);

عام:
    // Create new source AstCFile and open it for writing
    فراغ open_New_Output_منبع_File(ثابت std::string& پایه_Name, bool slow, bool support,
                                 ثابت حرف* descriptionp) {
        V3Stats::جمع_کن_Stat_جمع(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(پایه_Name, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    فراغ open_New_Output_سرآیند_File(ثابت std::string& پایه_Name, ثابت حرف* descriptionp) {
        openNewOutputFile(پایه_Name, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    فراغ openOutputFile(AstCFile* cfilep, ثابت حرف* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_تقسیم_اندازه = 0;
        إذا (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3_بیرون_CFile{VL_DEV_NULL};
            أرجع;
        }
        m_ofp = new V3_بیرون_CFile{cfilep->name()};
        puts_سرآیند();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    فراغ closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_تقسیم_اندازه);
        m_cfilep = nullptr;
    }

    std::وکتور<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        أرجع std::move(m_newCfileps);
    }

    فراغ تقسیم_اندازه_Inc(اندازه_t شمار) { m_تقسیم_اندازه += شمار; }
    فراغ تقسیم_اندازه_Inc(ثابت Ast_گره* nodep) {
        تقسیم_اندازه_Inc(static_cast<اندازه_t>(nodep->گره_شمار()));
    }
    bool تقسیم_Needed(اندازه_t تقسیم_حد) ثابت { أرجع m_تقسیم_اندازه >= تقسیم_حد; }
    bool تقسیم_Needed() ثابت { أرجع تقسیم_Needed(m_تقسیم_حد); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emit_تقسیم_Ctor_Dtor(متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, اندازه_t پایه_هزینه,
                      ثابت std::string& پیشوند, ثابت std::string& params,
                      ثابت std::string& آرگومانها, bool support, ثابت حرف* descriptionp,
                      ثابت std::function<فراغ()>& emit_سرآیند);

    // Returns pointer to current output file object.
    V3_بیرون_CFile* ofp() ثابت VL_MT_امن { أرجع m_ofp; }

    فراغ puts(ثابت string& str) { ofp()->puts(str); }
    فراغ putns(ثابت Ast_گره* nodep, ثابت string& str) { ofp()->putns(nodep, str); }
    فراغ puts_سرآیند() { ofp()->puts_سرآیند(); }
    فراغ putbs(ثابت string& str) { ofp()->putbs(str); }
    فراغ putnbs(ثابت Ast_گره* nodep, ثابت string& str) { ofp()->putnbs(nodep, str); }
    فراغ putsDecoration(ثابت Ast_گره* nodep, ثابت string& str) {
        إذا (v3Global.opt.decoration()) putns(nodep, str);
    }
    فراغ putsQuoted(ثابت string& str) { ofp()->putsQuoted(str); }
    فراغ ensure_New_سطر() { ofp()->ensure_New_سطر(); }
    bool optSystemC() { أرجع v3Global.opt.systemC(); }
    ساكن string protect(ثابت string& name) VL_MT_امن { أرجع VIdProtect::protect(name); }
    ساكن string funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_گره_Module* modp = nullptr);
    ساكن bool isExplicitlyConstructed(ثابت AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ثابت Ast_گره_Module* ثابت modp = EmitCParentModule::get(varp);
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
    string c_Func_آرگومانها(ثابت AstCFunc* nodep, bool cpp = true);
    فراغ emitCDefaultConstructor(ثابت Ast_گره_Module* modp);
    فراغ emit_CFunc_سرآیند(ثابت AstCFunc* funcp, ثابت Ast_گره_Module* modp, bool withScope);
    فراغ emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_گره_Module* modp, bool cLinkage = false);
    فراغ emitVarDecl(ثابت AstVar* nodep, bool asRef = false);
    فراغ emitVarAccessors(ثابت AstVar* nodep);
    قالب <typename T_Callable>
    ساكن فراغ forModCUse(ثابت Ast_گره_Module* modp, VUseType useType, T_Callable action) {
        لكل (ثابت Ast_گره* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            إذا (ثابت AstCUse* ثابت usep = VN_CAST(itemp, CUse)) {
                إذا (usep->useType().شامل_است_Any(useType)) {
                    إذا (usep->useType().شامل_است_Any(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::پیشوند_Name_Protect(usep) + ".h\"\n");
                        استمر;  // Forward declaration is not necessary
                    }
                    إذا (usep->useType().شامل_است_Any(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::پیشوند_Name_Protect(usep) + ";\n");
                    }
                }
            }
        }
    }
    فراغ emitModCUse(ثابت Ast_گره_Module* modp, VUseType useType);
    فراغ emitSystemCSection(ثابت Ast_گره_Module* modp, VSystemCSectionType type);
    ساكن std::pair<string, File_سطر*> scSection(ثابت Ast_گره_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = افتراضي;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        لكل (AstCFile* ثابت cfilep : m_newCfileps) v3Global.rootp()->جمع_کن_Filesp(cfilep);
    }
};

#پایان_اگر  // guard
