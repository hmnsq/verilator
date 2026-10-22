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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"

#ଅନ୍ତର୍ଭାଵ <cmath>
#ଅନ୍ତର୍ଭାଵ <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ଅଚଳ VNUser4InUse user4InUse;

ସର୍ବସାଧାରଣ:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ସ୍ଥିର ଅଚଳ Ast_ଗଣ୍ଠି_Module* get(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) VL_MT_STABLE {
        ଫେରନ୍ତୁ VN_AS(nodep->user4p(), ଗଣ୍ଠି_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
ସର୍ବସାଧାରଣ:
    ସ୍ଥିର string voidSelfAssign(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ଅଚଳ string className = prefixNameProtect(modp);
        ଫେରନ୍ତୁ className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ସ୍ଥିର string pchClassName() VL_MT_STABLE { ଫେରନ୍ତୁ v3Global.opt.prefix() + "__pch"; }
    ସ୍ଥିର string symClassName() VL_MT_STABLE {
        ଫେରନ୍ତୁ v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    ସ୍ଥିର string symClassVar() { ଫେରନ୍ତୁ symClassName() + "* __restrict vlSymsp"; }
    ସ୍ଥିର string symClassAssign() {
        ଫେରନ୍ତୁ symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ସ୍ଥିର string ଶୀର୍ଷ_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        ଫେରନ୍ତୁ v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ସ୍ଥିର string execGraphName(ଅଚଳ AstExecGraph* nodep) {
        ଫେରନ୍ତୁ ଶୀର୍ଷ_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ସ୍ଥିର std::string constPoolName(ଅଚଳ AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        ଫେରନ୍ତୁ ଶୀର୍ଷ_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ସ୍ଥିର string prefixNameProtect(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : ସର୍ବସାଧାରଣ VNVisitorConst {
ସର୍ବସାଧାରଣ:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    ସର୍ବସାଧାରଣ:
        ସଂରଚନା Stmt final {
            std::string m_text;  // Statement text
            size_t m_cost;  // Cost of statement, for splitting
        };

    ବ୍ୟକ୍ତିଗତ:
        std::vector<Stmt> m_stmts;  // Statements

    ସର୍ବସାଧାରଣ:
        // Add a statement
        ଶୂନ୍ୟ add(ଅଚଳ std::string& stmt) {
            ଅଚଳ size_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_ଗଣ୍ଠି::INSTR_ଗଣନା_CTOR_DTOR;
            m_stmts.ଠେଲ_back({stmt, cost});
        }
        size_t size() ଅଚଳ { ଫେରନ୍ତୁ m_stmts.size(); }
        ଅଚଳ Stmt& operator[](size_t i) ଅଚଳ { ଫେରନ୍ତୁ m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() ଅଚଳ { ଫେରନ୍ତୁ m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() ଅଚଳ { ଫେରନ୍ତୁ m_stmts.end(); }
    };

ବ୍ୟକ୍ତିଗତ:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    size_t m_splitSize = 0;  // Complexity of this file
    ଅଚଳ size_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<size_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<size_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    ଶୂନ୍ୟ openNewOutputFile(ଅଚଳ std::string& baseName, bool slow, bool support, bool source,
                           ଅଚଳ ଅକ୍ଷର* ଅଚଳ descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        ଯଦି (source) {
            ଯଦି (slow) fileName += "__Slow";
            fileName += ".cpp";
        } ଅନ୍ୟଥା {
            fileName += ".h";
        }
        AstCFile* ଅଚଳ cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, ଅଚଳ std::string& prefix,
                                   ଅଚଳ std::string& params, ଅଚଳ std::string& args,
                                   size_t maxCost, bool support, ଅଚଳ ଅକ୍ଷର* descriptionp,
                                   ଅଚଳ std::function<ଶୂନ୍ୟ()>& emitHeader);

ସର୍ବସାଧାରଣ:
    // Create new source AstCFile and open it for writing
    ଶୂନ୍ୟ openNewOutputSourceFile(ଅଚଳ std::string& baseName, bool slow, bool support,
                                 ଅଚଳ ଅକ୍ଷର* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    ଶୂନ୍ୟ openNewOutputHeaderFile(ଅଚଳ std::string& baseName, ଅଚଳ ଅକ୍ଷର* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    ଶୂନ୍ୟ openOutputFile(AstCFile* cfilep, ଅଚଳ ଅକ୍ଷର* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        ଯଦି (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            ଫେରନ୍ତୁ;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    ଶୂନ୍ୟ closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_splitSize);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        ଫେରନ୍ତୁ std::move(m_newCfileps);
    }

    ଶୂନ୍ୟ splitSizeInc(size_t ଗଣନା_2) { m_splitSize += ଗଣନା_2; }
    ଶୂନ୍ୟ splitSizeInc(ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
        splitSizeInc(static_cast<size_t>(nodep->ଗଣ୍ଠି_ଗଣନା()));
    }
    bool splitNeeded(size_t splitLimit) ଅଚଳ { ଫେରନ୍ତୁ m_splitSize >= splitLimit; }
    bool splitNeeded() ଅଚଳ { ଫେରନ୍ତୁ splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost,
                      ଅଚଳ std::string& prefix, ଅଚଳ std::string& params,
                      ଅଚଳ std::string& args, bool support, ଅଚଳ ଅକ୍ଷର* descriptionp,
                      ଅଚଳ std::function<ଶୂନ୍ୟ()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_ofp; }

    ଶୂନ୍ୟ puts(ଅଚଳ string& str) { ofp()->puts(str); }
    ଶୂନ୍ୟ putns(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& str) { ofp()->putns(nodep, str); }
    ଶୂନ୍ୟ putsHeader() { ofp()->putsHeader(); }
    ଶୂନ୍ୟ putbs(ଅଚଳ string& str) { ofp()->putbs(str); }
    ଶୂନ୍ୟ putnbs(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& str) { ofp()->putnbs(nodep, str); }
    ଶୂନ୍ୟ putsDecoration(ଅଚଳ Ast_ଗଣ୍ଠି* nodep, ଅଚଳ string& str) {
        ଯଦି (v3Global.opt.decoration()) putns(nodep, str);
    }
    ଶୂନ୍ୟ putsQuoted(ଅଚଳ string& str) { ofp()->putsQuoted(str); }
    ଶୂନ୍ୟ ensure_New_ଧାଡ଼ି() { ofp()->ensure_New_ଧାଡ଼ି(); }
    bool optSystemC() { ଫେରନ୍ତୁ v3Global.opt.systemC(); }
    ସ୍ଥିର string protect(ଅଚଳ string& name) VL_MT_SAFE { ଫେରନ୍ତୁ VIdProtect::protect(name); }
    ସ୍ଥିର string funcNameProtect(ଅଚଳ AstCFunc* nodep, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp = nullptr);
    ସ୍ଥିର bool isExplicitlyConstructed(ଅଚଳ AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ଅଚଳ Ast_ଗଣ୍ଠି_Module* ଅଚଳ modp = EmitCParentModule::get(varp);
        ଯଦି (!modp || !varp->isModelState()) ଫେରନ୍ତୁ false;
        // Classes use real constructors
        ଯଦି (VN_IS(modp, Class)) ଫେରନ୍ତୁ false;
        // Constant pool has special handling
        ଯଦି (modp->isConstPool()) ଫେରନ୍ତୁ false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        ଫେରନ୍ତୁ !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    ସ୍ଥିର std::string varName(ଅଚଳ AstVar* varp) VL_MT_STABLE {
        ଅଚଳ std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        ଯଦି (varp->isPrimaryIO()) ଫେରନ୍ତୁ "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        ଯଦି (isExplicitlyConstructed(varp)) ଫେରନ୍ତୁ "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        ଫେରନ୍ତୁ name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(ଅଚଳ AstCFunc* nodep, bool cpp = true);
    ଶୂନ୍ୟ emitCDefaultConstructor(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp);
    ଶୂନ୍ୟ emitCFuncHeader(ଅଚଳ AstCFunc* funcp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, bool withScope);
    ଶୂନ୍ୟ emitCFuncDecl(ଅଚଳ AstCFunc* funcp, ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, bool cLinkage = false);
    ଶୂନ୍ୟ emitVarDecl(ଅଚଳ AstVar* nodep, bool asRef = false);
    ଶୂନ୍ୟ emitVarAccessors(ଅଚଳ AstVar* nodep);
    template <typename T_Callable>
    ସ୍ଥିର ଶୂନ୍ୟ forModCUse(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, VUseType useType, T_Callable action) {
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ଯଦି (ଅଚଳ AstCUse* ଅଚଳ usep = VN_CAST(itemp, CUse)) {
                ଯଦି (usep->useType().containsAny(useType)) {
                    ଯଦି (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        ଜାରି;  // Forward declaration is not necessary
                    }
                    ଯଦି (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    ଶୂନ୍ୟ emitModCUse(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, VUseType useType);
    ଶୂନ୍ୟ emitSystemCSection(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp, VSystemCSectionType type);
    ସ୍ଥିର std::pair<string, File_ଧାଡ଼ି*> scSection(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        ପାଇଁ (AstCFile* ଅଚଳ cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
