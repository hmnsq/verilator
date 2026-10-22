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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3File.h"

#അന്തര്ഭാവ <cmath>
#അന്തര്ഭാവ <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    സ്ഥിരം VNUser4InUse user4InUse;

പൊതു:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    സ്ഥിതം സ്ഥിരം Ast_കണ്ണി_Module* get(സ്ഥിരം Ast_കണ്ണി* nodep) VL_MT_STABLE {
        തിരികെ VN_AS(nodep->user4p(), കണ്ണി_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
പൊതു:
    സ്ഥിതം string voidSelfAssign(സ്ഥിരം Ast_കണ്ണി_Module* modp) {
        സ്ഥിരം string className = prefixNameProtect(modp);
        തിരികെ className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    സ്ഥിതം string pchClassName() VL_MT_STABLE { തിരികെ v3Global.opt.prefix() + "__pch"; }
    സ്ഥിതം string symClassName() VL_MT_STABLE {
        തിരികെ v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    സ്ഥിതം string symClassVar() { തിരികെ symClassName() + "* __restrict vlSymsp"; }
    സ്ഥിതം string symClassAssign() {
        തിരികെ symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    സ്ഥിതം string മുകൾ_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        തിരികെ v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    സ്ഥിതം string execGraphName(സ്ഥിരം AstExecGraph* nodep) {
        തിരികെ മുകൾ_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    സ്ഥിതം std::string constPoolName(സ്ഥിരം AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        തിരികെ മുകൾ_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    സ്ഥിതം string prefixNameProtect(സ്ഥിരം Ast_കണ്ണി* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : പൊതു VNVisitorConst {
പൊതു:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    പൊതു:
        ഘടന Stmt final {
            std::string m_text;  // Statement text
            size_t m_cost;  // Cost of statement, for splitting
        };

    സ്വകാര്യ:
        std::vector<Stmt> m_stmts;  // Statements

    പൊതു:
        // Add a statement
        ശൂന്യം add(സ്ഥിരം std::string& stmt) {
            സ്ഥിരം size_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_കണ്ണി::INSTR_എണ്ണം_CTOR_DTOR;
            m_stmts.തള്ളുക_back({stmt, cost});
        }
        size_t size() സ്ഥിരം { തിരികെ m_stmts.size(); }
        സ്ഥിരം Stmt& operator[](size_t i) സ്ഥിരം { തിരികെ m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() സ്ഥിരം { തിരികെ m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() സ്ഥിരം { തിരികെ m_stmts.end(); }
    };

സ്വകാര്യ:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    size_t m_splitSize = 0;  // Complexity of this file
    സ്ഥിരം size_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<size_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<size_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    ശൂന്യം openNewOutputFile(സ്ഥിരം std::string& baseName, bool slow, bool support, bool source,
                           സ്ഥിരം അക്ഷരം* സ്ഥിരം descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        എങ്കിൽ (source) {
            എങ്കിൽ (slow) fileName += "__Slow";
            fileName += ".cpp";
        } അല്ലെങ്കിൽ {
            fileName += ".h";
        }
        AstCFile* സ്ഥിരം cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, സ്ഥിരം std::string& prefix,
                                   സ്ഥിരം std::string& params, സ്ഥിരം std::string& args,
                                   size_t maxCost, bool support, സ്ഥിരം അക്ഷരം* descriptionp,
                                   സ്ഥിരം std::function<ശൂന്യം()>& emitHeader);

പൊതു:
    // Create new source AstCFile and open it for writing
    ശൂന്യം openNewOutputSourceFile(സ്ഥിരം std::string& baseName, bool slow, bool support,
                                 സ്ഥിരം അക്ഷരം* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    ശൂന്യം openNewOutputHeaderFile(സ്ഥിരം std::string& baseName, സ്ഥിരം അക്ഷരം* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    ശൂന്യം openOutputFile(AstCFile* cfilep, സ്ഥിരം അക്ഷരം* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        എങ്കിൽ (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            തിരികെ;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    ശൂന്യം closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_splitSize);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        തിരികെ std::move(m_newCfileps);
    }

    ശൂന്യം splitSizeInc(size_t എണ്ണം) { m_splitSize += എണ്ണം; }
    ശൂന്യം splitSizeInc(സ്ഥിരം Ast_കണ്ണി* nodep) {
        splitSizeInc(static_cast<size_t>(nodep->കണ്ണി_എണ്ണം()));
    }
    bool splitNeeded(size_t splitLimit) സ്ഥിരം { തിരികെ m_splitSize >= splitLimit; }
    bool splitNeeded() സ്ഥിരം { തിരികെ splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost,
                      സ്ഥിരം std::string& prefix, സ്ഥിരം std::string& params,
                      സ്ഥിരം std::string& args, bool support, സ്ഥിരം അക്ഷരം* descriptionp,
                      സ്ഥിരം std::function<ശൂന്യം()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() സ്ഥിരം VL_MT_SAFE { തിരികെ m_ofp; }

    ശൂന്യം puts(സ്ഥിരം string& str) { ofp()->puts(str); }
    ശൂന്യം putns(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& str) { ofp()->putns(nodep, str); }
    ശൂന്യം putsHeader() { ofp()->putsHeader(); }
    ശൂന്യം putbs(സ്ഥിരം string& str) { ofp()->putbs(str); }
    ശൂന്യം putnbs(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& str) { ofp()->putnbs(nodep, str); }
    ശൂന്യം putsDecoration(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം string& str) {
        എങ്കിൽ (v3Global.opt.decoration()) putns(nodep, str);
    }
    ശൂന്യം putsQuoted(സ്ഥിരം string& str) { ofp()->putsQuoted(str); }
    ശൂന്യം ensure_New_വരി() { ofp()->ensure_New_വരി(); }
    bool optSystemC() { തിരികെ v3Global.opt.systemC(); }
    സ്ഥിതം string protect(സ്ഥിരം string& name) VL_MT_SAFE { തിരികെ VIdProtect::protect(name); }
    സ്ഥിതം string funcNameProtect(സ്ഥിരം AstCFunc* nodep, സ്ഥിരം Ast_കണ്ണി_Module* modp = nullptr);
    സ്ഥിതം bool isExplicitlyConstructed(സ്ഥിരം AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം modp = EmitCParentModule::get(varp);
        എങ്കിൽ (!modp || !varp->isModelState()) തിരികെ false;
        // Classes use real constructors
        എങ്കിൽ (VN_IS(modp, Class)) തിരികെ false;
        // Constant pool has special handling
        എങ്കിൽ (modp->isConstPool()) തിരികെ false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        തിരികെ !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    സ്ഥിതം std::string varName(സ്ഥിരം AstVar* varp) VL_MT_STABLE {
        സ്ഥിരം std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        എങ്കിൽ (varp->isPrimaryIO()) തിരികെ "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        എങ്കിൽ (isExplicitlyConstructed(varp)) തിരികെ "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        തിരികെ name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(സ്ഥിരം AstCFunc* nodep, bool cpp = true);
    ശൂന്യം emitCDefaultConstructor(സ്ഥിരം Ast_കണ്ണി_Module* modp);
    ശൂന്യം emitCFuncHeader(സ്ഥിരം AstCFunc* funcp, സ്ഥിരം Ast_കണ്ണി_Module* modp, bool withScope);
    ശൂന്യം emitCFuncDecl(സ്ഥിരം AstCFunc* funcp, സ്ഥിരം Ast_കണ്ണി_Module* modp, bool cLinkage = false);
    ശൂന്യം emitVarDecl(സ്ഥിരം AstVar* nodep, bool asRef = false);
    ശൂന്യം emitVarAccessors(സ്ഥിരം AstVar* nodep);
    template <typename T_Callable>
    സ്ഥിതം ശൂന്യം forModCUse(സ്ഥിരം Ast_കണ്ണി_Module* modp, VUseType useType, T_Callable action) {
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (സ്ഥിരം AstCUse* സ്ഥിരം usep = VN_CAST(itemp, CUse)) {
                എങ്കിൽ (usep->useType().containsAny(useType)) {
                    എങ്കിൽ (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        തുടരുക;  // Forward declaration is not necessary
                    }
                    എങ്കിൽ (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    ശൂന്യം emitModCUse(സ്ഥിരം Ast_കണ്ണി_Module* modp, VUseType useType);
    ശൂന്യം emitSystemCSection(സ്ഥിരം Ast_കണ്ണി_Module* modp, VSystemCSectionType type);
    സ്ഥിതം std::pair<string, File_വരി*> scSection(സ്ഥിരം Ast_കണ്ണി_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = സ്ഥിരസ്ഥിതി;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        വേണ്ടി (AstCFile* സ്ഥിരം cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
