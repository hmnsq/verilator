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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3EMITCBASE_H_
#وصف_ڪريو VERILATOR_V3EMITCBASE_H_

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
    ثابت VNUser4_اندريون_Use user4_اندريون_Use;

عام:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ساكن ثابت Ast_ڳنڍ_Module* get(ثابت Ast_ڳنڍ* nodep) VL_MT_STABLE {
        أرجع VN_AS(nodep->user4p(), ڳنڍ_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
عام:
    ساكن string void_Self_سونپيو(ثابت Ast_ڳنڍ_Module* modp) {
        ثابت string className = اڳياڙي_Name_Protect(modp);
        أرجع className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ساكن string pchClassName() VL_MT_STABLE { أرجع v3Global.opt.اڳياڙي() + "__pch"; }
    ساكن string symClassName() VL_MT_STABLE {
        أرجع v3Global.opt.اڳياڙي() + "_" + VIdProtect::protect("_Syms");
    }
    ساكن string symClassVar() { أرجع symClassName() + "* __restrict vlSymsp"; }
    ساكن string sym_Class_سونپيو() {
        أرجع symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ساكن string چوٽي_Class_Name() VL_MT_محفوظ {  // Return name of top wrapper module
        أرجع v3Global.opt.اڳياڙي();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ساكن string exec_گراف_Name(ثابت Ast_Exec_گراف* nodep) {
        أرجع چوٽي_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ساكن std::string constPoolName(ثابت AstVar* varp) {
        UASSERT_OBJ(varp->const_Pool_داخلا(), varp, "Not a constant pool variable");
        أرجع چوٽي_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ساكن string اڳياڙي_Name_Protect(ثابت Ast_ڳنڍ* nodep) VL_MT_STABLE;
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
            سائيز_t m_لاڳت;  // Cost of statement, for splitting
        };

    خاص:
        std::ويڪٽر<Stmt> m_stmts;  // Statements

    عام:
        // Add a statement
        فراغ جوڙيو(ثابت std::string& stmt) {
            ثابت سائيز_t لاڳت = stmt.خالي_جايون() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_ڳنڍ::INSTR_ڳڻپ_CTOR_DTOR;
            m_stmts.ڌڪو_پٺ({stmt, لاڳت});
        }
        سائيز_t سائيز() ثابت { أرجع m_stmts.سائيز(); }
        ثابت Stmt& عامل[](سائيز_t i) ثابت { أرجع m_stmts[i]; }
        std::ويڪٽر<Stmt>::const_iterator begin() ثابت { أرجع m_stmts.begin(); }
        std::ويڪٽر<Stmt>::const_iterator پڇاڙي() ثابت { أرجع m_stmts.پڇاڙي(); }
    };

خاص:
    // STATE
    V3_ٻاهريون_CFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::ويڪٽر<AstCFile*> m_newCfileps;  // AstCFiles created
    سائيز_t m_ورهايو_سائيز = 0;  // Complexity of this file
    ثابت سائيز_t m_ورهايو_حد_بندي = v3Global.opt.output_ورهايو()
                                    ? static_cast<سائيز_t>(v3Global.opt.output_ورهايو())
                                    : std::numeric_limits<سائيز_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    فراغ openNewOutputFile(ثابت std::string& بنياد_Name, bool slow, bool support, bool ذريعو,
                           ثابت حرف* ثابت descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + بنياد_Name;
        إذا (ذريعو) {
            إذا (slow) fileName += "__Slow";
            fileName += ".cpp";
        } وإلا {
            fileName += ".h";
        }
        AstCFile* ثابت cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->ذريعو(ذريعو);
        cfilep->support(support);
        m_newCfileps.emplace_پٺ(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emit_ورهايو_متن_Stmts(متن_Stmts& stmts, ثابت std::string& اڳياڙي,
                                   ثابت std::string& params, ثابت std::string& دليلون,
                                   سائيز_t max_لاڳت, bool support, ثابت حرف* descriptionp,
                                   ثابت std::function<فراغ()>& emit_سرو);

عام:
    // Create new source AstCFile and open it for writing
    فراغ open_New_Output_ذريعو_File(ثابت std::string& بنياد_Name, bool slow, bool support,
                                 ثابت حرف* descriptionp) {
        V3Stats::جوڙيو_Stat_جوڙ(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(بنياد_Name, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    فراغ open_New_Output_سرو_File(ثابت std::string& بنياد_Name, ثابت حرف* descriptionp) {
        openNewOutputFile(بنياد_Name, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    فراغ openOutputFile(AstCFile* cfilep, ثابت حرف* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_ورهايو_سائيز = 0;
        إذا (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3_ٻاهريون_CFile{VL_DEV_NULL};
            أرجع;
        }
        m_ofp = new V3_ٻاهريون_CFile{cfilep->name()};
        puts_سرو();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    فراغ closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_ورهايو_سائيز);
        m_cfilep = nullptr;
    }

    std::ويڪٽر<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        أرجع std::move(m_newCfileps);
    }

    فراغ ورهايو_سائيز_Inc(سائيز_t ڳڻپ) { m_ورهايو_سائيز += ڳڻپ; }
    فراغ ورهايو_سائيز_Inc(ثابت Ast_ڳنڍ* nodep) {
        ورهايو_سائيز_Inc(static_cast<سائيز_t>(nodep->ڳنڍ_ڳڻپ()));
    }
    bool ورهايو_Needed(سائيز_t ورهايو_حد_بندي) ثابت { أرجع m_ورهايو_سائيز >= ورهايو_حد_بندي; }
    bool ورهايو_Needed() ثابت { أرجع ورهايو_Needed(m_ورهايو_حد_بندي); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emit_ورهايو_Ctor_Dtor(متن_Stmts& ctorStmts, متن_Stmts& dtorStmts, سائيز_t بنياد_لاڳت,
                      ثابت std::string& اڳياڙي, ثابت std::string& params,
                      ثابت std::string& دليلون, bool support, ثابت حرف* descriptionp,
                      ثابت std::function<فراغ()>& emit_سرو);

    // Returns pointer to current output file object.
    V3_ٻاهريون_CFile* ofp() ثابت VL_MT_محفوظ { أرجع m_ofp; }

    فراغ puts(ثابت string& str) { ofp()->puts(str); }
    فراغ putns(ثابت Ast_ڳنڍ* nodep, ثابت string& str) { ofp()->putns(nodep, str); }
    فراغ puts_سرو() { ofp()->puts_سرو(); }
    فراغ putbs(ثابت string& str) { ofp()->putbs(str); }
    فراغ putnbs(ثابت Ast_ڳنڍ* nodep, ثابت string& str) { ofp()->putnbs(nodep, str); }
    فراغ putsDecoration(ثابت Ast_ڳنڍ* nodep, ثابت string& str) {
        إذا (v3Global.opt.decoration()) putns(nodep, str);
    }
    فراغ putsQuoted(ثابت string& str) { ofp()->putsQuoted(str); }
    فراغ ensure_New_سٽ() { ofp()->ensure_New_سٽ(); }
    bool optSystemC() { أرجع v3Global.opt.systemC(); }
    ساكن string protect(ثابت string& name) VL_MT_محفوظ { أرجع VIdProtect::protect(name); }
    ساكن string funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_ڳنڍ_Module* modp = nullptr);
    ساكن bool isExplicitlyConstructed(ثابت AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ثابت Ast_ڳنڍ_Module* ثابت modp = EmitCParentModule::get(varp);
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
    string c_Func_دليلون(ثابت AstCFunc* nodep, bool cpp = true);
    فراغ emitCDefaultConstructor(ثابت Ast_ڳنڍ_Module* modp);
    فراغ emit_CFunc_سرو(ثابت AstCFunc* funcp, ثابت Ast_ڳنڍ_Module* modp, bool withScope);
    فراغ emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_ڳنڍ_Module* modp, bool cLinkage = false);
    فراغ emitVarDecl(ثابت AstVar* nodep, bool asRef = false);
    فراغ emitVarAccessors(ثابت AstVar* nodep);
    سانچو <typename T_Callable>
    ساكن فراغ forModCUse(ثابت Ast_ڳنڍ_Module* modp, VUseType useType, T_Callable action) {
        لكل (ثابت Ast_ڳنڍ* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            إذا (ثابت AstCUse* ثابت usep = VN_CAST(itemp, CUse)) {
                إذا (usep->useType().شامل_آهي_Any(useType)) {
                    إذا (usep->useType().شامل_آهي_Any(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::اڳياڙي_Name_Protect(usep) + ".h\"\n");
                        استمر;  // Forward declaration is not necessary
                    }
                    إذا (usep->useType().شامل_آهي_Any(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::اڳياڙي_Name_Protect(usep) + ";\n");
                    }
                }
            }
        }
    }
    فراغ emitModCUse(ثابت Ast_ڳنڍ_Module* modp, VUseType useType);
    فراغ emitSystemCSection(ثابت Ast_ڳنڍ_Module* modp, VSystemCSectionType type);
    ساكن std::pair<string, File_سٽ*> scSection(ثابت Ast_ڳنڍ_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = افتراضي;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        لكل (AstCFile* ثابت cfilep : m_newCfileps) v3Global.rootp()->جوڙيو_Filesp(cfilep);
    }
};

#جيڪڏهن_پورو  // guard
