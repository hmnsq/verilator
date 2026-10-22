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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3File.h"

#अन्तर्भाव <cmath>
#अन्तर्भाव <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ध्रुव VNUser4InUse user4InUse;

सार्वजनिक:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    स्थिर ध्रुव Ast_ग्रन्थिः_Module* get(ध्रुव Ast_ग्रन्थिः* nodep) VL_MT_STABLE {
        प्रत्यागम VN_AS(nodep->user4p(), ग्रन्थिः_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
सार्वजनिक:
    स्थिर string voidSelfAssign(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        ध्रुव string className = prefixNameProtect(modp);
        प्रत्यागम className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    स्थिर string pchClassName() VL_MT_STABLE { प्रत्यागम v3Global.opt.prefix() + "__pch"; }
    स्थिर string symClassName() VL_MT_STABLE {
        प्रत्यागम v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    स्थिर string symClassVar() { प्रत्यागम symClassName() + "* __restrict vlSymsp"; }
    स्थिर string symClassAssign() {
        प्रत्यागम symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    स्थिर string शिखरम्_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        प्रत्यागम v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    स्थिर string execGraphName(ध्रुव AstExecGraph* nodep) {
        प्रत्यागम शिखरम्_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    स्थिर std::string constPoolName(ध्रुव AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        प्रत्यागम शिखरम्_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    स्थिर string prefixNameProtect(ध्रुव Ast_ग्रन्थिः* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : सार्वजनिक VNVisitorConst {
सार्वजनिक:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    सार्वजनिक:
        संरचना Stmt final {
            std::string m_text;  // Statement text
            size_t m_cost;  // Cost of statement, for splitting
        };

    गुप्त:
        std::vector<Stmt> m_stmts;  // Statements

    सार्वजनिक:
        // Add a statement
        शून्य add(ध्रुव std::string& stmt) {
            ध्रुव size_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_ग्रन्थिः::INSTR_गणनम्_CTOR_DTOR;
            m_stmts.प्रक्षिप_back({stmt, cost});
        }
        size_t size() ध्रुव { प्रत्यागम m_stmts.size(); }
        ध्रुव Stmt& operator[](size_t i) ध्रुव { प्रत्यागम m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() ध्रुव { प्रत्यागम m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() ध्रुव { प्रत्यागम m_stmts.end(); }
    };

गुप्त:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    size_t m_splitSize = 0;  // Complexity of this file
    ध्रुव size_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<size_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<size_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    शून्य openNewOutputFile(ध्रुव std::string& baseName, bool slow, bool support, bool source,
                           ध्रुव वर्ण* ध्रुव descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        जइ (source) {
            जइ (slow) fileName += "__Slow";
            fileName += ".cpp";
        } अण्णहा {
            fileName += ".h";
        }
        AstCFile* ध्रुव cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, ध्रुव std::string& prefix,
                                   ध्रुव std::string& params, ध्रुव std::string& args,
                                   size_t maxCost, bool support, ध्रुव वर्ण* descriptionp,
                                   ध्रुव std::function<शून्य()>& emitHeader);

सार्वजनिक:
    // Create new source AstCFile and open it for writing
    शून्य openNewOutputSourceFile(ध्रुव std::string& baseName, bool slow, bool support,
                                 ध्रुव वर्ण* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    शून्य openNewOutputHeaderFile(ध्रुव std::string& baseName, ध्रुव वर्ण* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    शून्य openOutputFile(AstCFile* cfilep, ध्रुव वर्ण* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        जइ (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            प्रत्यागम;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    शून्य closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_splitSize);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        प्रत्यागम std::move(m_newCfileps);
    }

    शून्य splitSizeInc(size_t गणनम्) { m_splitSize += गणनम्; }
    शून्य splitSizeInc(ध्रुव Ast_ग्रन्थिः* nodep) {
        splitSizeInc(static_cast<size_t>(nodep->ग्रन्थिः_गणनम्()));
    }
    bool splitNeeded(size_t splitLimit) ध्रुव { प्रत्यागम m_splitSize >= splitLimit; }
    bool splitNeeded() ध्रुव { प्रत्यागम splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost,
                      ध्रुव std::string& prefix, ध्रुव std::string& params,
                      ध्रुव std::string& args, bool support, ध्रुव वर्ण* descriptionp,
                      ध्रुव std::function<शून्य()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() ध्रुव VL_MT_SAFE { प्रत्यागम m_ofp; }

    शून्य puts(ध्रुव string& str) { ofp()->puts(str); }
    शून्य putns(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& str) { ofp()->putns(nodep, str); }
    शून्य putsHeader() { ofp()->putsHeader(); }
    शून्य putbs(ध्रुव string& str) { ofp()->putbs(str); }
    शून्य putnbs(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& str) { ofp()->putnbs(nodep, str); }
    शून्य putsDecoration(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव string& str) {
        जइ (v3Global.opt.decoration()) putns(nodep, str);
    }
    शून्य putsQuoted(ध्रुव string& str) { ofp()->putsQuoted(str); }
    शून्य ensure_New_पङ्क्तिः() { ofp()->ensure_New_पङ्क्तिः(); }
    bool optSystemC() { प्रत्यागम v3Global.opt.systemC(); }
    स्थिर string protect(ध्रुव string& name) VL_MT_SAFE { प्रत्यागम VIdProtect::protect(name); }
    स्थिर string funcNameProtect(ध्रुव AstCFunc* nodep, ध्रुव Ast_ग्रन्थिः_Module* modp = nullptr);
    स्थिर bool isExplicitlyConstructed(ध्रुव AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ध्रुव Ast_ग्रन्थिः_Module* ध्रुव modp = EmitCParentModule::get(varp);
        जइ (!modp || !varp->isModelState()) प्रत्यागम false;
        // Classes use real constructors
        जइ (VN_IS(modp, Class)) प्रत्यागम false;
        // Constant pool has special handling
        जइ (modp->isConstPool()) प्रत्यागम false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        प्रत्यागम !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    स्थिर std::string varName(ध्रुव AstVar* varp) VL_MT_STABLE {
        ध्रुव std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        जइ (varp->isPrimaryIO()) प्रत्यागम "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        जइ (isExplicitlyConstructed(varp)) प्रत्यागम "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        प्रत्यागम name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(ध्रुव AstCFunc* nodep, bool cpp = true);
    शून्य emitCDefaultConstructor(ध्रुव Ast_ग्रन्थिः_Module* modp);
    शून्य emitCFuncHeader(ध्रुव AstCFunc* funcp, ध्रुव Ast_ग्रन्थिः_Module* modp, bool withScope);
    शून्य emitCFuncDecl(ध्रुव AstCFunc* funcp, ध्रुव Ast_ग्रन्थिः_Module* modp, bool cLinkage = false);
    शून्य emitVarDecl(ध्रुव AstVar* nodep, bool asRef = false);
    शून्य emitVarAccessors(ध्रुव AstVar* nodep);
    template <typename T_Callable>
    स्थिर शून्य forModCUse(ध्रुव Ast_ग्रन्थिः_Module* modp, VUseType useType, T_Callable action) {
        प्रति (ध्रुव Ast_ग्रन्थिः* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            जइ (ध्रुव AstCUse* ध्रुव usep = VN_CAST(itemp, CUse)) {
                जइ (usep->useType().containsAny(useType)) {
                    जइ (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        अनुवर्त;  // Forward declaration is not necessary
                    }
                    जइ (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    शून्य emitModCUse(ध्रुव Ast_ग्रन्थिः_Module* modp, VUseType useType);
    शून्य emitSystemCSection(ध्रुव Ast_ग्रन्थिः_Module* modp, VSystemCSectionType type);
    स्थिर std::pair<string, File_पङ्क्तिः*> scSection(ध्रुव Ast_ग्रन्थिः_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = स्वभाव;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        प्रति (AstCFile* ध्रुव cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
