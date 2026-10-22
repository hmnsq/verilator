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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3File.h"

#ಅನ್ತರ್ಭಾವ <cmath>
#ಅನ್ತರ್ಭಾವ <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ಧ್ರುವ VNUser4InUse user4InUse;

ಸಾರ್ವಜನಿಕ:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ಸ್ಥಿರ ಧ್ರುವ Ast_ಗಂಟು_Module* get(ಧ್ರುವ Ast_ಗಂಟು* nodep) VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ VN_AS(nodep->user4p(), ಗಂಟು_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ string voidSelfAssign(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಧ್ರುವ string className = prefixNameProtect(modp);
        ಪ್ರತ್ಯಾಗಮ className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ಸ್ಥಿರ string pchClassName() VL_MT_STABLE { ಪ್ರತ್ಯಾಗಮ v3Global.opt.prefix() + "__pch"; }
    ಸ್ಥಿರ string symClassName() VL_MT_STABLE {
        ಪ್ರತ್ಯಾಗಮ v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    ಸ್ಥಿರ string symClassVar() { ಪ್ರತ್ಯಾಗಮ symClassName() + "* __restrict vlSymsp"; }
    ಸ್ಥಿರ string symClassAssign() {
        ಪ್ರತ್ಯಾಗಮ symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ಸ್ಥಿರ string ತುದಿ_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        ಪ್ರತ್ಯಾಗಮ v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ಸ್ಥಿರ string execGraphName(ಧ್ರುವ AstExecGraph* nodep) {
        ಪ್ರತ್ಯಾಗಮ ತುದಿ_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ಸ್ಥಿರ std::string constPoolName(ಧ್ರುವ AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        ಪ್ರತ್ಯಾಗಮ ತುದಿ_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ಸ್ಥಿರ string prefixNameProtect(ಧ್ರುವ Ast_ಗಂಟು* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitorConst {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    ಸಾರ್ವಜನಿಕ:
        ಸಂರಚನಾ Stmt final {
            std::string m_text;  // Statement text
            size_t m_cost;  // Cost of statement, for splitting
        };

    ಗುಪ್ತ:
        std::vector<Stmt> m_stmts;  // Statements

    ಸಾರ್ವಜನಿಕ:
        // Add a statement
        ಶೂನ್ಯ add(ಧ್ರುವ std::string& stmt) {
            ಧ್ರುವ size_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_ಗಂಟು::INSTR_ಎಣಿಕೆ_CTOR_DTOR;
            m_stmts.ತಳ್ಳು_back({stmt, cost});
        }
        size_t size() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stmts.size(); }
        ಧ್ರುವ Stmt& operator[](size_t i) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stmts.end(); }
    };

ಗುಪ್ತ:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    size_t m_splitSize = 0;  // Complexity of this file
    ಧ್ರುವ size_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<size_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<size_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    ಶೂನ್ಯ openNewOutputFile(ಧ್ರುವ std::string& baseName, bool slow, bool support, bool source,
                           ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        ಯದಿ (source) {
            ಯದಿ (slow) fileName += "__Slow";
            fileName += ".cpp";
        } ಅನ್ಯಥಾ {
            fileName += ".h";
        }
        AstCFile* ಧ್ರುವ cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, ಧ್ರುವ std::string& prefix,
                                   ಧ್ರುವ std::string& params, ಧ್ರುವ std::string& args,
                                   size_t maxCost, bool support, ಧ್ರುವ ವರ್ಣ* descriptionp,
                                   ಧ್ರುವ std::function<ಶೂನ್ಯ()>& emitHeader);

ಸಾರ್ವಜನಿಕ:
    // Create new source AstCFile and open it for writing
    ಶೂನ್ಯ openNewOutputSourceFile(ಧ್ರುವ std::string& baseName, bool slow, bool support,
                                 ಧ್ರುವ ವರ್ಣ* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    ಶೂನ್ಯ openNewOutputHeaderFile(ಧ್ರುವ std::string& baseName, ಧ್ರುವ ವರ್ಣ* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    ಶೂನ್ಯ openOutputFile(AstCFile* cfilep, ಧ್ರುವ ವರ್ಣ* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        ಯದಿ (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            ಪ್ರತ್ಯಾಗಮ;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    ಶೂನ್ಯ closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_splitSize);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        ಪ್ರತ್ಯಾಗಮ std::move(m_newCfileps);
    }

    ಶೂನ್ಯ splitSizeInc(size_t ಎಣಿಕೆ) { m_splitSize += ಎಣಿಕೆ; }
    ಶೂನ್ಯ splitSizeInc(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        splitSizeInc(static_cast<size_t>(nodep->ಗಂಟು_ಎಣಿಕೆ()));
    }
    bool splitNeeded(size_t splitLimit) ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_splitSize >= splitLimit; }
    bool splitNeeded() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost,
                      ಧ್ರುವ std::string& prefix, ಧ್ರುವ std::string& params,
                      ಧ್ರುವ std::string& args, bool support, ಧ್ರುವ ವರ್ಣ* descriptionp,
                      ಧ್ರುವ std::function<ಶೂನ್ಯ()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_ofp; }

    ಶೂನ್ಯ puts(ಧ್ರುವ string& str) { ofp()->puts(str); }
    ಶೂನ್ಯ putns(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& str) { ofp()->putns(nodep, str); }
    ಶೂನ್ಯ putsHeader() { ofp()->putsHeader(); }
    ಶೂನ್ಯ putbs(ಧ್ರುವ string& str) { ofp()->putbs(str); }
    ಶೂನ್ಯ putnbs(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& str) { ofp()->putnbs(nodep, str); }
    ಶೂನ್ಯ putsDecoration(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ string& str) {
        ಯದಿ (v3Global.opt.decoration()) putns(nodep, str);
    }
    ಶೂನ್ಯ putsQuoted(ಧ್ರುವ string& str) { ofp()->putsQuoted(str); }
    ಶೂನ್ಯ ensure_New_ಸಾಲು() { ofp()->ensure_New_ಸಾಲು(); }
    bool optSystemC() { ಪ್ರತ್ಯಾಗಮ v3Global.opt.systemC(); }
    ಸ್ಥಿರ string protect(ಧ್ರುವ string& name) VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ VIdProtect::protect(name); }
    ಸ್ಥಿರ string funcNameProtect(ಧ್ರುವ AstCFunc* nodep, ಧ್ರುವ Ast_ಗಂಟು_Module* modp = nullptr);
    ಸ್ಥಿರ bool isExplicitlyConstructed(ಧ್ರುವ AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ modp = EmitCParentModule::get(varp);
        ಯದಿ (!modp || !varp->isModelState()) ಪ್ರತ್ಯಾಗಮ false;
        // Classes use real constructors
        ಯದಿ (VN_IS(modp, Class)) ಪ್ರತ್ಯಾಗಮ false;
        // Constant pool has special handling
        ಯದಿ (modp->isConstPool()) ಪ್ರತ್ಯಾಗಮ false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        ಪ್ರತ್ಯಾಗಮ !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    ಸ್ಥಿರ std::string varName(ಧ್ರುವ AstVar* varp) VL_MT_STABLE {
        ಧ್ರುವ std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        ಯದಿ (varp->isPrimaryIO()) ಪ್ರತ್ಯಾಗಮ "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        ಯದಿ (isExplicitlyConstructed(varp)) ಪ್ರತ್ಯಾಗಮ "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        ಪ್ರತ್ಯಾಗಮ name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(ಧ್ರುವ AstCFunc* nodep, bool cpp = true);
    ಶೂನ್ಯ emitCDefaultConstructor(ಧ್ರುವ Ast_ಗಂಟು_Module* modp);
    ಶೂನ್ಯ emitCFuncHeader(ಧ್ರುವ AstCFunc* funcp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp, bool withScope);
    ಶೂನ್ಯ emitCFuncDecl(ಧ್ರುವ AstCFunc* funcp, ಧ್ರುವ Ast_ಗಂಟು_Module* modp, bool cLinkage = false);
    ಶೂನ್ಯ emitVarDecl(ಧ್ರುವ AstVar* nodep, bool asRef = false);
    ಶೂನ್ಯ emitVarAccessors(ಧ್ರುವ AstVar* nodep);
    template <typename T_Callable>
    ಸ್ಥಿರ ಶೂನ್ಯ forModCUse(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, VUseType useType, T_Callable action) {
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstCUse* ಧ್ರುವ usep = VN_CAST(itemp, CUse)) {
                ಯದಿ (usep->useType().containsAny(useType)) {
                    ಯದಿ (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        ಅನುವರ್ತ;  // Forward declaration is not necessary
                    }
                    ಯದಿ (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ emitModCUse(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, VUseType useType);
    ಶೂನ್ಯ emitSystemCSection(ಧ್ರುವ Ast_ಗಂಟು_Module* modp, VSystemCSectionType type);
    ಸ್ಥಿರ std::pair<string, File_ಸಾಲು*> scSection(ಧ್ರುವ Ast_ಗಂಟು_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = ಸ್ವಭಾವ;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        ಪ್ರತಿ (AstCFile* ಧ್ರುವ cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
