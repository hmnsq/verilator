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
    स्थिरांक VNUser4InUse user4InUse;

सार्वजनिक:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    स्थिर स्थिरांक Ast_गाठ_Module* get(स्थिरांक Ast_गाठ* nodep) VL_MT_STABLE {
        परत VN_AS(nodep->user4p(), गाठ_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
सार्वजनिक:
    स्थिर string voidSelfAssign(स्थिरांक Ast_गाठ_Module* modp) {
        स्थिरांक string className = prefixNameProtect(modp);
        परत className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    स्थिर string pchClassName() VL_MT_STABLE { परत v3Global.opt.prefix() + "__pch"; }
    स्थिर string symClassName() VL_MT_STABLE {
        परत v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    स्थिर string symClassVar() { परत symClassName() + "* __restrict vlSymsp"; }
    स्थिर string symClassAssign() {
        परत symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    स्थिर string शीर्ष_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        परत v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    स्थिर string execGraphName(स्थिरांक AstExecGraph* nodep) {
        परत शीर्ष_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    स्थिर std::string constPoolName(स्थिरांक AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        परत शीर्ष_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    स्थिर string prefixNameProtect(स्थिरांक Ast_गाठ* nodep) VL_MT_STABLE;
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
            माप_प्रकार m_cost;  // Cost of statement, for splitting
        };

    खाजगी:
        std::vector<Stmt> m_stmts;  // Statements

    सार्वजनिक:
        // Add a statement
        शून्य add(स्थिरांक std::string& stmt) {
            स्थिरांक माप_प्रकार cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_गाठ::INSTR_मोजणी_CTOR_DTOR;
            m_stmts.ढकला_back({stmt, cost});
        }
        माप_प्रकार size() स्थिरांक { परत m_stmts.size(); }
        स्थिरांक Stmt& चालक[](माप_प्रकार i) स्थिरांक { परत m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() स्थिरांक { परत m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() स्थिरांक { परत m_stmts.end(); }
    };

खाजगी:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    माप_प्रकार m_splitSize = 0;  // Complexity of this file
    स्थिरांक माप_प्रकार m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<माप_प्रकार>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<माप_प्रकार>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    शून्य openNewOutputFile(स्थिरांक std::string& baseName, bool slow, bool support, bool source,
                           स्थिरांक अक्षर* स्थिरांक descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        जर (source) {
            जर (slow) fileName += "__Slow";
            fileName += ".cpp";
        } अन्यथा {
            fileName += ".h";
        }
        AstCFile* स्थिरांक cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, स्थिरांक std::string& prefix,
                                   स्थिरांक std::string& params, स्थिरांक std::string& args,
                                   माप_प्रकार maxCost, bool support, स्थिरांक अक्षर* descriptionp,
                                   स्थिरांक std::function<शून्य()>& emitHeader);

सार्वजनिक:
    // Create new source AstCFile and open it for writing
    शून्य openNewOutputSourceFile(स्थिरांक std::string& baseName, bool slow, bool support,
                                 स्थिरांक अक्षर* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    शून्य openNewOutputHeaderFile(स्थिरांक std::string& baseName, स्थिरांक अक्षर* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    शून्य openOutputFile(AstCFile* cfilep, स्थिरांक अक्षर* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        जर (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            परत;
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
        परत std::move(m_newCfileps);
    }

    शून्य splitSizeInc(माप_प्रकार मोजणी) { m_splitSize += मोजणी; }
    शून्य splitSizeInc(स्थिरांक Ast_गाठ* nodep) {
        splitSizeInc(static_cast<माप_प्रकार>(nodep->गाठ_मोजणी()));
    }
    bool splitNeeded(माप_प्रकार splitLimit) स्थिरांक { परत m_splitSize >= splitLimit; }
    bool splitNeeded() स्थिरांक { परत splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, माप_प्रकार baseCost,
                      स्थिरांक std::string& prefix, स्थिरांक std::string& params,
                      स्थिरांक std::string& args, bool support, स्थिरांक अक्षर* descriptionp,
                      स्थिरांक std::function<शून्य()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() स्थिरांक VL_MT_SAFE { परत m_ofp; }

    शून्य माला_दो(स्थिरांक string& str) { ofp()->माला_दो(str); }
    शून्य putns(स्थिरांक Ast_गाठ* nodep, स्थिरांक string& str) { ofp()->putns(nodep, str); }
    शून्य putsHeader() { ofp()->putsHeader(); }
    शून्य putbs(स्थिरांक string& str) { ofp()->putbs(str); }
    शून्य putnbs(स्थिरांक Ast_गाठ* nodep, स्थिरांक string& str) { ofp()->putnbs(nodep, str); }
    शून्य putsDecoration(स्थिरांक Ast_गाठ* nodep, स्थिरांक string& str) {
        जर (v3Global.opt.decoration()) putns(nodep, str);
    }
    शून्य putsQuoted(स्थिरांक string& str) { ofp()->putsQuoted(str); }
    शून्य ensure_New_ओळ() { ofp()->ensure_New_ओळ(); }
    bool optSystemC() { परत v3Global.opt.systemC(); }
    स्थिर string protect(स्थिरांक string& name) VL_MT_SAFE { परत VIdProtect::protect(name); }
    स्थिर string funcNameProtect(स्थिरांक AstCFunc* nodep, स्थिरांक Ast_गाठ_Module* modp = nullptr);
    स्थिर bool isExplicitlyConstructed(स्थिरांक AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        स्थिरांक Ast_गाठ_Module* स्थिरांक modp = EmitCParentModule::get(varp);
        जर (!modp || !varp->isModelState()) परत false;
        // Classes use real constructors
        जर (VN_IS(modp, Class)) परत false;
        // Constant pool has special handling
        जर (modp->isConstPool()) परत false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        परत !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    स्थिर std::string varName(स्थिरांक AstVar* varp) VL_MT_STABLE {
        स्थिरांक std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        जर (varp->isPrimaryIO()) परत "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        जर (isExplicitlyConstructed(varp)) परत "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        परत name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(स्थिरांक AstCFunc* nodep, bool cpp = true);
    शून्य emitCDefaultConstructor(स्थिरांक Ast_गाठ_Module* modp);
    शून्य emitCFuncHeader(स्थिरांक AstCFunc* funcp, स्थिरांक Ast_गाठ_Module* modp, bool withScope);
    शून्य emitCFuncDecl(स्थिरांक AstCFunc* funcp, स्थिरांक Ast_गाठ_Module* modp, bool cLinkage = false);
    शून्य emitVarDecl(स्थिरांक AstVar* nodep, bool asRef = false);
    शून्य emitVarAccessors(स्थिरांक AstVar* nodep);
    ढाँचा <typename T_Callable>
    स्थिर शून्य forModCUse(स्थिरांक Ast_गाठ_Module* modp, VUseType useType, T_Callable action) {
        साठी (स्थिरांक Ast_गाठ* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            जर (स्थिरांक AstCUse* स्थिरांक usep = VN_CAST(itemp, CUse)) {
                जर (usep->useType().containsAny(useType)) {
                    जर (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        सुरूठेवा;  // Forward declaration is not necessary
                    }
                    जर (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    शून्य emitModCUse(स्थिरांक Ast_गाठ_Module* modp, VUseType useType);
    शून्य emitSystemCSection(स्थिरांक Ast_गाठ_Module* modp, VSystemCSectionType type);
    स्थिर std::pair<string, File_ओळ*> scSection(स्थिरांक Ast_गाठ_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = पूर्वनिर्धारित;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        साठी (AstCFile* स्थिरांक cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
