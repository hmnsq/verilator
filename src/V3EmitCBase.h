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

#अगर_अघोषित VERILATOR_V3EMITCBASE_H_
#घोषणा VERILATOR_V3EMITCBASE_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3File.h"

#समावेश <cmath>
#समावेश <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    स्थिर VNUser4InUse user4InUse;

खुला:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    अटल स्थिर Ast_गाँठ_Module* get(स्थिर Ast_गाँठ* nodep) VL_MT_STABLE {
        वापस VN_AS(nodep->user4p(), गाँठ_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
खुला:
    अटल string voidSelfAssign(स्थिर Ast_गाँठ_Module* modp) {
        स्थिर string className = prefixNameProtect(modp);
        वापस className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    अटल string pchClassName() VL_MT_STABLE { वापस v3Global.opt.prefix() + "__pch"; }
    अटल string symClassName() VL_MT_STABLE {
        वापस v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    अटल string symClassVar() { वापस symClassName() + "* __restrict vlSymsp"; }
    अटल string symClassAssign() {
        वापस symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    अटल string शीर्ष_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        वापस v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    अटल string execGraphName(स्थिर AstExecGraph* nodep) {
        वापस शीर्ष_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    अटल std::string constPoolName(स्थिर AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        वापस शीर्ष_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    अटल string prefixNameProtect(स्थिर Ast_गाँठ* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : खुला VNVisitorConst {
खुला:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    खुला:
        काष्ठा Stmt final {
            std::string m_text;  // Statement text
            माप_प्रकार m_cost;  // Cost of statement, for splitting
        };

    निजी:
        std::vector<Stmt> m_stmts;  // Statements

    खुला:
        // Add a statement
        व्योम add(स्थिर std::string& stmt) {
            स्थिर माप_प्रकार cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_गाँठ::INSTR_गिनती_CTOR_DTOR;
            m_stmts.धकेलो_back({stmt, cost});
        }
        माप_प्रकार आकार() स्थिर { वापस m_stmts.आकार(); }
        स्थिर Stmt& चालक[](माप_प्रकार i) स्थिर { वापस m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() स्थिर { वापस m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() स्थिर { वापस m_stmts.end(); }
    };

निजी:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    माप_प्रकार m_split_आकार = 0;  // Complexity of this file
    स्थिर माप_प्रकार m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<माप_प्रकार>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<माप_प्रकार>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    व्योम openNewOutputFile(स्थिर std::string& baseName, bool slow, bool support, bool source,
                           स्थिर अक्षर* स्थिर descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        अगर (source) {
            अगर (slow) fileName += "__Slow";
            fileName += ".cpp";
        } अन्यथा {
            fileName += ".h";
        }
        AstCFile* स्थिर cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, स्थिर std::string& prefix,
                                   स्थिर std::string& params, स्थिर std::string& args,
                                   माप_प्रकार maxCost, bool support, स्थिर अक्षर* descriptionp,
                                   स्थिर std::function<व्योम()>& emitHeader);

खुला:
    // Create new source AstCFile and open it for writing
    व्योम openNewOutputSourceFile(स्थिर std::string& baseName, bool slow, bool support,
                                 स्थिर अक्षर* descriptionp) {
        V3Stats::add_Stat_योग(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    व्योम openNewOutputHeaderFile(स्थिर std::string& baseName, स्थिर अक्षर* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    व्योम openOutputFile(AstCFile* cfilep, स्थिर अक्षर* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_split_आकार = 0;
        अगर (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            वापस;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    व्योम closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_split_आकार);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        वापस std::move(m_newCfileps);
    }

    व्योम split_आकार_Inc(माप_प्रकार गिनती) { m_split_आकार += गिनती; }
    व्योम split_आकार_Inc(स्थिर Ast_गाँठ* nodep) {
        split_आकार_Inc(static_cast<माप_प्रकार>(nodep->गाँठ_गिनती()));
    }
    bool splitNeeded(माप_प्रकार splitLimit) स्थिर { वापस m_split_आकार >= splitLimit; }
    bool splitNeeded() स्थिर { वापस splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, माप_प्रकार baseCost,
                      स्थिर std::string& prefix, स्थिर std::string& params,
                      स्थिर std::string& args, bool support, स्थिर अक्षर* descriptionp,
                      स्थिर std::function<व्योम()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() स्थिर VL_MT_SAFE { वापस m_ofp; }

    व्योम माला_दो(स्थिर string& str) { ofp()->माला_दो(str); }
    व्योम putns(स्थिर Ast_गाँठ* nodep, स्थिर string& str) { ofp()->putns(nodep, str); }
    व्योम putsHeader() { ofp()->putsHeader(); }
    व्योम putbs(स्थिर string& str) { ofp()->putbs(str); }
    व्योम putnbs(स्थिर Ast_गाँठ* nodep, स्थिर string& str) { ofp()->putnbs(nodep, str); }
    व्योम putsDecoration(स्थिर Ast_गाँठ* nodep, स्थिर string& str) {
        अगर (v3Global.opt.decoration()) putns(nodep, str);
    }
    व्योम putsQuoted(स्थिर string& str) { ofp()->putsQuoted(str); }
    व्योम ensure_New_पंक्ति() { ofp()->ensure_New_पंक्ति(); }
    bool optSystemC() { वापस v3Global.opt.systemC(); }
    अटल string protect(स्थिर string& name) VL_MT_SAFE { वापस VIdProtect::protect(name); }
    अटल string funcNameProtect(स्थिर AstCFunc* nodep, स्थिर Ast_गाँठ_Module* modp = nullptr);
    अटल bool isExplicitlyConstructed(स्थिर AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        स्थिर Ast_गाँठ_Module* स्थिर modp = EmitCParentModule::get(varp);
        अगर (!modp || !varp->isModelState()) वापस false;
        // Classes use real constructors
        अगर (VN_IS(modp, Class)) वापस false;
        // Constant pool has special handling
        अगर (modp->isConstPool()) वापस false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        वापस !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    अटल std::string varName(स्थिर AstVar* varp) VL_MT_STABLE {
        स्थिर std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        अगर (varp->isPrimaryIO()) वापस "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        अगर (isExplicitlyConstructed(varp)) वापस "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        वापस name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(स्थिर AstCFunc* nodep, bool cpp = true);
    व्योम emitCDefaultConstructor(स्थिर Ast_गाँठ_Module* modp);
    व्योम emitCFuncHeader(स्थिर AstCFunc* funcp, स्थिर Ast_गाँठ_Module* modp, bool withScope);
    व्योम emitCFuncDecl(स्थिर AstCFunc* funcp, स्थिर Ast_गाँठ_Module* modp, bool cLinkage = false);
    व्योम emitVarDecl(स्थिर AstVar* nodep, bool asRef = false);
    व्योम emitVarAccessors(स्थिर AstVar* nodep);
    ढाँचा <typename T_Callable>
    अटल व्योम forModCUse(स्थिर Ast_गाँठ_Module* modp, VUseType useType, T_Callable action) {
        क्रम (स्थिर Ast_गाँठ* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            अगर (स्थिर AstCUse* स्थिर usep = VN_CAST(itemp, CUse)) {
                अगर (usep->useType().containsAny(useType)) {
                    अगर (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        जारी;  // Forward declaration is not necessary
                    }
                    अगर (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    व्योम emitModCUse(स्थिर Ast_गाँठ_Module* modp, VUseType useType);
    व्योम emitSystemCSection(स्थिर Ast_गाँठ_Module* modp, VSystemCSectionType type);
    अटल std::pair<string, File_पंक्ति*> scSection(स्थिर Ast_गाँठ_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = शेष;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        क्रम (AstCFile* स्थिर cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#पूर्ण_अगर  // guard
