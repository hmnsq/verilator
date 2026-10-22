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

#ifndef VERILATOR_V3EMITCBASE_H_
#define VERILATOR_V3EMITCBASE_H_

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
    ثابت VNUser4InUse user4InUse;

عمومی:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ایستا ثابت Ast_گره_Module* get(ثابت Ast_گره* nodep) VL_MT_STABLE {
        بازگشت VN_AS(nodep->user4p(), گره_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
عمومی:
    ایستا string voidSelfAssign(ثابت Ast_گره_Module* modp) {
        ثابت string className = prefixNameProtect(modp);
        بازگشت className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ایستا string pchClassName() VL_MT_STABLE { بازگشت v3Global.opt.prefix() + "__pch"; }
    ایستا string symClassName() VL_MT_STABLE {
        بازگشت v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    ایستا string symClassVar() { بازگشت symClassName() + "* __restrict vlSymsp"; }
    ایستا string symClassAssign() {
        بازگشت symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ایستا string بالاترین_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        بازگشت v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ایستا string execGraphName(ثابت AstExecGraph* nodep) {
        بازگشت بالاترین_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ایستا std::string constPoolName(ثابت AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        بازگشت بالاترین_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ایستا string prefixNameProtect(ثابت Ast_گره* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : عمومی VNVisitorConst {
عمومی:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    عمومی:
        ساختار Stmt final {
            std::string m_text;  // Statement text
            size_t m_cost;  // Cost of statement, for splitting
        };

    خصوصی:
        std::vector<Stmt> m_stmts;  // Statements

    عمومی:
        // Add a statement
        تهی add(ثابت std::string& stmt) {
            ثابت size_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_گره::INSTR_شمار_CTOR_DTOR;
            m_stmts.افزودن_back({stmt, cost});
        }
        size_t size() ثابت { بازگشت m_stmts.size(); }
        ثابت Stmt& operator[](size_t i) ثابت { بازگشت m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() ثابت { بازگشت m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() ثابت { بازگشت m_stmts.end(); }
    };

خصوصی:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    size_t m_splitSize = 0;  // Complexity of this file
    ثابت size_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<size_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<size_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    تهی openNewOutputFile(ثابت std::string& baseName, bool slow, bool support, bool source,
                           ثابت نویسه* ثابت descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        اگر (source) {
            اگر (slow) fileName += "__Slow";
            fileName += ".cpp";
        } وگرنه {
            fileName += ".h";
        }
        AstCFile* ثابت cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, ثابت std::string& prefix,
                                   ثابت std::string& params, ثابت std::string& args,
                                   size_t maxCost, bool support, ثابت نویسه* descriptionp,
                                   ثابت std::function<تهی()>& emitHeader);

عمومی:
    // Create new source AstCFile and open it for writing
    تهی openNewOutputSourceFile(ثابت std::string& baseName, bool slow, bool support,
                                 ثابت نویسه* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    تهی openNewOutputHeaderFile(ثابت std::string& baseName, ثابت نویسه* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    تهی openOutputFile(AstCFile* cfilep, ثابت نویسه* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        اگر (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            بازگشت;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    تهی closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_splitSize);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        بازگشت std::move(m_newCfileps);
    }

    تهی splitSizeInc(size_t شمار) { m_splitSize += شمار; }
    تهی splitSizeInc(ثابت Ast_گره* nodep) {
        splitSizeInc(static_cast<size_t>(nodep->گره_شمار()));
    }
    bool splitNeeded(size_t splitLimit) ثابت { بازگشت m_splitSize >= splitLimit; }
    bool splitNeeded() ثابت { بازگشت splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost,
                      ثابت std::string& prefix, ثابت std::string& params,
                      ثابت std::string& args, bool support, ثابت نویسه* descriptionp,
                      ثابت std::function<تهی()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() ثابت VL_MT_SAFE { بازگشت m_ofp; }

    تهی puts(ثابت string& str) { ofp()->puts(str); }
    تهی putns(ثابت Ast_گره* nodep, ثابت string& str) { ofp()->putns(nodep, str); }
    تهی putsHeader() { ofp()->putsHeader(); }
    تهی putbs(ثابت string& str) { ofp()->putbs(str); }
    تهی putnbs(ثابت Ast_گره* nodep, ثابت string& str) { ofp()->putnbs(nodep, str); }
    تهی putsDecoration(ثابت Ast_گره* nodep, ثابت string& str) {
        اگر (v3Global.opt.decoration()) putns(nodep, str);
    }
    تهی putsQuoted(ثابت string& str) { ofp()->putsQuoted(str); }
    تهی ensure_New_سطر() { ofp()->ensure_New_سطر(); }
    bool optSystemC() { بازگشت v3Global.opt.systemC(); }
    ایستا string protect(ثابت string& name) VL_MT_SAFE { بازگشت VIdProtect::protect(name); }
    ایستا string funcNameProtect(ثابت AstCFunc* nodep, ثابت Ast_گره_Module* modp = nullptr);
    ایستا bool isExplicitlyConstructed(ثابت AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ثابت Ast_گره_Module* ثابت modp = EmitCParentModule::get(varp);
        اگر (!modp || !varp->isModelState()) بازگشت false;
        // Classes use real constructors
        اگر (VN_IS(modp, Class)) بازگشت false;
        // Constant pool has special handling
        اگر (modp->isConstPool()) بازگشت false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        بازگشت !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    ایستا std::string varName(ثابت AstVar* varp) VL_MT_STABLE {
        ثابت std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        اگر (varp->isPrimaryIO()) بازگشت "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        اگر (isExplicitlyConstructed(varp)) بازگشت "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        بازگشت name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(ثابت AstCFunc* nodep, bool cpp = true);
    تهی emitCDefaultConstructor(ثابت Ast_گره_Module* modp);
    تهی emitCFuncHeader(ثابت AstCFunc* funcp, ثابت Ast_گره_Module* modp, bool withScope);
    تهی emitCFuncDecl(ثابت AstCFunc* funcp, ثابت Ast_گره_Module* modp, bool cLinkage = false);
    تهی emitVarDecl(ثابت AstVar* nodep, bool asRef = false);
    تهی emitVarAccessors(ثابت AstVar* nodep);
    template <typename T_Callable>
    ایستا تهی forModCUse(ثابت Ast_گره_Module* modp, VUseType useType, T_Callable action) {
        برای (ثابت Ast_گره* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            اگر (ثابت AstCUse* ثابت usep = VN_CAST(itemp, CUse)) {
                اگر (usep->useType().containsAny(useType)) {
                    اگر (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        ادامه;  // Forward declaration is not necessary
                    }
                    اگر (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    تهی emitModCUse(ثابت Ast_گره_Module* modp, VUseType useType);
    تهی emitSystemCSection(ثابت Ast_گره_Module* modp, VSystemCSectionType type);
    ایستا std::pair<string, File_سطر*> scSection(ثابت Ast_گره_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = پیشفرض;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        برای (AstCFile* ثابت cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
