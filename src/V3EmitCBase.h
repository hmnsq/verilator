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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3File.h"

#অন্তর্ভাৱ <cmath>
#অন্তর্ভাৱ <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ধ্রুৱ VNUser4InUse user4InUse;

সার্ৱজনিক:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    স্থির ধ্রুৱ Ast_গ্ৰন্থি_Module* get(ধ্রুৱ Ast_গ্ৰন্থি* nodep) VL_MT_STABLE {
        ঘূৰাই VN_AS(nodep->user4p(), গ্ৰন্থি_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
সার্ৱজনিক:
    স্থির string voidSelfAssign(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp) {
        ধ্রুৱ string className = prefixNameProtect(modp);
        ঘূৰাই className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    স্থির string pchClassName() VL_MT_STABLE { ঘূৰাই v3Global.opt.prefix() + "__pch"; }
    স্থির string symClassName() VL_MT_STABLE {
        ঘূৰাই v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    স্থির string symClassVar() { ঘূৰাই symClassName() + "* __restrict vlSymsp"; }
    স্থির string symClassAssign() {
        ঘূৰাই symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    স্থির string শীৰ্ষ_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        ঘূৰাই v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    স্থির string execGraphName(ধ্রুৱ AstExecGraph* nodep) {
        ঘূৰাই শীৰ্ষ_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    স্থির std::string constPoolName(ধ্রুৱ AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        ঘূৰাই শীৰ্ষ_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    স্থির string prefixNameProtect(ধ্রুৱ Ast_গ্ৰন্থি* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : সার্ৱজনিক VNVisitorConst {
সার্ৱজনিক:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    সার্ৱজনিক:
        সংরচনা Stmt final {
            std::string m_text;  // Statement text
            size_t m_cost;  // Cost of statement, for splitting
        };

    গুপ্ত:
        std::vector<Stmt> m_stmts;  // Statements

    সার্ৱজনিক:
        // Add a statement
        শূন্য add(ধ্রুৱ std::string& stmt) {
            ধ্রুৱ size_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_গ্ৰন্থি::INSTR_গুনতি_CTOR_DTOR;
            m_stmts.ঠেলো_back({stmt, cost});
        }
        size_t size() ধ্রুৱ { ঘূৰাই m_stmts.size(); }
        ধ্রুৱ Stmt& operator[](size_t i) ধ্রুৱ { ঘূৰাই m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() ধ্রুৱ { ঘূৰাই m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() ধ্রুৱ { ঘূৰাই m_stmts.end(); }
    };

গুপ্ত:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    size_t m_splitSize = 0;  // Complexity of this file
    ধ্রুৱ size_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<size_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<size_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    শূন্য openNewOutputFile(ধ্রুৱ std::string& baseName, bool slow, bool support, bool source,
                           ধ্রুৱ ৱর্ণ* ধ্রুৱ descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        যদি (source) {
            যদি (slow) fileName += "__Slow";
            fileName += ".cpp";
        } অন্যথা {
            fileName += ".h";
        }
        AstCFile* ধ্রুৱ cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, ধ্রুৱ std::string& prefix,
                                   ধ্রুৱ std::string& params, ধ্রুৱ std::string& args,
                                   size_t maxCost, bool support, ধ্রুৱ ৱর্ণ* descriptionp,
                                   ধ্রুৱ std::function<শূন্য()>& emitHeader);

সার্ৱজনিক:
    // Create new source AstCFile and open it for writing
    শূন্য openNewOutputSourceFile(ধ্রুৱ std::string& baseName, bool slow, bool support,
                                 ধ্রুৱ ৱর্ণ* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    শূন্য openNewOutputHeaderFile(ধ্রুৱ std::string& baseName, ধ্রুৱ ৱর্ণ* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    শূন্য openOutputFile(AstCFile* cfilep, ধ্রুৱ ৱর্ণ* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        যদি (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            ঘূৰাই;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    শূন্য closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_splitSize);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        ঘূৰাই std::move(m_newCfileps);
    }

    শূন্য splitSizeInc(size_t গুনতি) { m_splitSize += গুনতি; }
    শূন্য splitSizeInc(ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
        splitSizeInc(static_cast<size_t>(nodep->গ্ৰন্থি_গুনতি()));
    }
    bool splitNeeded(size_t splitLimit) ধ্রুৱ { ঘূৰাই m_splitSize >= splitLimit; }
    bool splitNeeded() ধ্রুৱ { ঘূৰাই splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost,
                      ধ্রুৱ std::string& prefix, ধ্রুৱ std::string& params,
                      ধ্রুৱ std::string& args, bool support, ধ্রুৱ ৱর্ণ* descriptionp,
                      ধ্রুৱ std::function<শূন্য()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() ধ্রুৱ VL_MT_SAFE { ঘূৰাই m_ofp; }

    শূন্য puts(ধ্রুৱ string& str) { ofp()->puts(str); }
    শূন্য putns(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& str) { ofp()->putns(nodep, str); }
    শূন্য putsHeader() { ofp()->putsHeader(); }
    শূন্য putbs(ধ্রুৱ string& str) { ofp()->putbs(str); }
    শূন্য putnbs(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& str) { ofp()->putnbs(nodep, str); }
    শূন্য putsDecoration(ধ্রুৱ Ast_গ্ৰন্থি* nodep, ধ্রুৱ string& str) {
        যদি (v3Global.opt.decoration()) putns(nodep, str);
    }
    শূন্য putsQuoted(ধ্রুৱ string& str) { ofp()->putsQuoted(str); }
    শূন্য ensure_New_সাৰি() { ofp()->ensure_New_সাৰি(); }
    bool optSystemC() { ঘূৰাই v3Global.opt.systemC(); }
    স্থির string protect(ধ্রুৱ string& name) VL_MT_SAFE { ঘূৰাই VIdProtect::protect(name); }
    স্থির string funcNameProtect(ধ্রুৱ AstCFunc* nodep, ধ্রুৱ Ast_গ্ৰন্থি_Module* modp = nullptr);
    স্থির bool isExplicitlyConstructed(ধ্রুৱ AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ধ্রুৱ Ast_গ্ৰন্থি_Module* ধ্রুৱ modp = EmitCParentModule::get(varp);
        যদি (!modp || !varp->isModelState()) ঘূৰাই false;
        // Classes use real constructors
        যদি (VN_IS(modp, Class)) ঘূৰাই false;
        // Constant pool has special handling
        যদি (modp->isConstPool()) ঘূৰাই false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        ঘূৰাই !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    স্থির std::string varName(ধ্রুৱ AstVar* varp) VL_MT_STABLE {
        ধ্রুৱ std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        যদি (varp->isPrimaryIO()) ঘূৰাই "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        যদি (isExplicitlyConstructed(varp)) ঘূৰাই "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        ঘূৰাই name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(ধ্রুৱ AstCFunc* nodep, bool cpp = true);
    শূন্য emitCDefaultConstructor(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp);
    শূন্য emitCFuncHeader(ধ্রুৱ AstCFunc* funcp, ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, bool withScope);
    শূন্য emitCFuncDecl(ধ্রুৱ AstCFunc* funcp, ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, bool cLinkage = false);
    শূন্য emitVarDecl(ধ্রুৱ AstVar* nodep, bool asRef = false);
    শূন্য emitVarAccessors(ধ্রুৱ AstVar* nodep);
    template <typename T_Callable>
    স্থির শূন্য forModCUse(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, VUseType useType, T_Callable action) {
        বাবে (ধ্রুৱ Ast_গ্ৰন্থি* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            যদি (ধ্রুৱ AstCUse* ধ্রুৱ usep = VN_CAST(itemp, CUse)) {
                যদি (usep->useType().containsAny(useType)) {
                    যদি (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        অনুৱর্ত;  // Forward declaration is not necessary
                    }
                    যদি (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    শূন্য emitModCUse(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, VUseType useType);
    শূন্য emitSystemCSection(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp, VSystemCSectionType type);
    স্থির std::pair<string, File_সাৰি*> scSection(ধ্রুৱ Ast_গ্ৰন্থি_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = স্ৱভাৱ;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        বাবে (AstCFile* ধ্রুৱ cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
