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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3File.h"

#অন্তর্ভাব <cmath>
#অন্তর্ভাব <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    ধ্রুবক VNUser4InUse user4InUse;

সর্বজনীন:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    স্থির ধ্রুবক Ast_গ্রন্থি_Module* get(ধ্রুবক Ast_গ্রন্থি* nodep) VL_MT_STABLE {
        ফেরত VN_AS(nodep->user4p(), গ্রন্থি_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
সর্বজনীন:
    স্থির string voidSelfAssign(ধ্রুবক Ast_গ্রন্থি_Module* modp) {
        ধ্রুবক string className = prefixNameProtect(modp);
        ফেরত className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    স্থির string pchClassName() VL_MT_STABLE { ফেরত v3Global.opt.prefix() + "__pch"; }
    স্থির string symClassName() VL_MT_STABLE {
        ফেরত v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    স্থির string symClassVar() { ফেরত symClassName() + "* __restrict vlSymsp"; }
    স্থির string symClassAssign() {
        ফেরত symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    স্থির string শীর্ষ_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        ফেরত v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    স্থির string execGraphName(ধ্রুবক AstExecGraph* nodep) {
        ফেরত শীর্ষ_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    স্থির std::string constPoolName(ধ্রুবক AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        ফেরত শীর্ষ_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    স্থির string prefixNameProtect(ধ্রুবক Ast_গ্রন্থি* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : সর্বজনীন VNVisitorConst {
সর্বজনীন:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    সর্বজনীন:
        কাঠামো Stmt final {
            std::string m_text;  // Statement text
            আকার_t m_cost;  // Cost of statement, for splitting
        };

    ব্যক্তিগত:
        std::vector<Stmt> m_stmts;  // Statements

    সর্বজনীন:
        // Add a statement
        শূন্য add(ধ্রুবক std::string& stmt) {
            ধ্রুবক আকার_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_গ্রন্থি::INSTR_গুনতি_CTOR_DTOR;
            m_stmts.ঠেলো_back({stmt, cost});
        }
        আকার_t আকার_2() ধ্রুবক { ফেরত m_stmts.আকার_2(); }
        ধ্রুবক Stmt& operator[](আকার_t i) ধ্রুবক { ফেরত m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() ধ্রুবক { ফেরত m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() ধ্রুবক { ফেরত m_stmts.end(); }
    };

ব্যক্তিগত:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    আকার_t m_split_আকার = 0;  // Complexity of this file
    ধ্রুবক আকার_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<আকার_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<আকার_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    শূন্য openNewOutputFile(ধ্রুবক std::string& baseName, bool slow, bool support, bool source,
                           ধ্রুবক অক্ষর* ধ্রুবক descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        যদি (source) {
            যদি (slow) fileName += "__Slow";
            fileName += ".cpp";
        } অন্যথা {
            fileName += ".h";
        }
        AstCFile* ধ্রুবক cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, ধ্রুবক std::string& prefix,
                                   ধ্রুবক std::string& params, ধ্রুবক std::string& args,
                                   আকার_t maxCost, bool support, ধ্রুবক অক্ষর* descriptionp,
                                   ধ্রুবক std::function<শূন্য()>& emitHeader);

সর্বজনীন:
    // Create new source AstCFile and open it for writing
    শূন্য openNewOutputSourceFile(ধ্রুবক std::string& baseName, bool slow, bool support,
                                 ধ্রুবক অক্ষর* descriptionp) {
        V3Stats::add_Stat_যোগ(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    শূন্য openNewOutputHeaderFile(ধ্রুবক std::string& baseName, ধ্রুবক অক্ষর* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    শূন্য openOutputFile(AstCFile* cfilep, ধ্রুবক অক্ষর* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_split_আকার = 0;
        যদি (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            ফেরত;
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
        m_cfilep->complexityScore(m_split_আকার);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        ফেরত std::move(m_newCfileps);
    }

    শূন্য split_আকার_Inc(আকার_t গুনতি) { m_split_আকার += গুনতি; }
    শূন্য split_আকার_Inc(ধ্রুবক Ast_গ্রন্থি* nodep) {
        split_আকার_Inc(static_cast<আকার_t>(nodep->গ্রন্থি_গুনতি()));
    }
    bool splitNeeded(আকার_t splitLimit) ধ্রুবক { ফেরত m_split_আকার >= splitLimit; }
    bool splitNeeded() ধ্রুবক { ফেরত splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, আকার_t baseCost,
                      ধ্রুবক std::string& prefix, ধ্রুবক std::string& params,
                      ধ্রুবক std::string& args, bool support, ধ্রুবক অক্ষর* descriptionp,
                      ধ্রুবক std::function<শূন্য()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() ধ্রুবক VL_MT_SAFE { ফেরত m_ofp; }

    শূন্য puts(ধ্রুবক string& str) { ofp()->puts(str); }
    শূন্য putns(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক string& str) { ofp()->putns(nodep, str); }
    শূন্য putsHeader() { ofp()->putsHeader(); }
    শূন্য putbs(ধ্রুবক string& str) { ofp()->putbs(str); }
    শূন্য putnbs(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক string& str) { ofp()->putnbs(nodep, str); }
    শূন্য putsDecoration(ধ্রুবক Ast_গ্রন্থি* nodep, ধ্রুবক string& str) {
        যদি (v3Global.opt.decoration()) putns(nodep, str);
    }
    শূন্য putsQuoted(ধ্রুবক string& str) { ofp()->putsQuoted(str); }
    শূন্য ensure_New_সারি() { ofp()->ensure_New_সারি(); }
    bool optSystemC() { ফেরত v3Global.opt.systemC(); }
    স্থির string protect(ধ্রুবক string& name) VL_MT_SAFE { ফেরত VIdProtect::protect(name); }
    স্থির string funcNameProtect(ধ্রুবক AstCFunc* nodep, ধ্রুবক Ast_গ্রন্থি_Module* modp = nullptr);
    স্থির bool isExplicitlyConstructed(ধ্রুবক AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ধ্রুবক Ast_গ্রন্থি_Module* ধ্রুবক modp = EmitCParentModule::get(varp);
        যদি (!modp || !varp->isModelState()) ফেরত false;
        // Classes use real constructors
        যদি (VN_IS(modp, Class)) ফেরত false;
        // Constant pool has special handling
        যদি (modp->isConstPool()) ফেরত false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        ফেরত !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    স্থির std::string varName(ধ্রুবক AstVar* varp) VL_MT_STABLE {
        ধ্রুবক std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        যদি (varp->isPrimaryIO()) ফেরত "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        যদি (isExplicitlyConstructed(varp)) ফেরত "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        ফেরত name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(ধ্রুবক AstCFunc* nodep, bool cpp = true);
    শূন্য emitCDefaultConstructor(ধ্রুবক Ast_গ্রন্থি_Module* modp);
    শূন্য emitCFuncHeader(ধ্রুবক AstCFunc* funcp, ধ্রুবক Ast_গ্রন্থি_Module* modp, bool withScope);
    শূন্য emitCFuncDecl(ধ্রুবক AstCFunc* funcp, ধ্রুবক Ast_গ্রন্থি_Module* modp, bool cLinkage = false);
    শূন্য emitVarDecl(ধ্রুবক AstVar* nodep, bool asRef = false);
    শূন্য emitVarAccessors(ধ্রুবক AstVar* nodep);
    template <typename T_Callable>
    স্থির শূন্য forModCUse(ধ্রুবক Ast_গ্রন্থি_Module* modp, VUseType useType, T_Callable action) {
        জন্য (ধ্রুবক Ast_গ্রন্থি* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            যদি (ধ্রুবক AstCUse* ধ্রুবক usep = VN_CAST(itemp, CUse)) {
                যদি (usep->useType().containsAny(useType)) {
                    যদি (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        চালু;  // Forward declaration is not necessary
                    }
                    যদি (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    শূন্য emitModCUse(ধ্রুবক Ast_গ্রন্থি_Module* modp, VUseType useType);
    শূন্য emitSystemCSection(ধ্রুবক Ast_গ্রন্থি_Module* modp, VSystemCSectionType type);
    স্থির std::pair<string, File_সারি*> scSection(ধ্রুবক Ast_গ্রন্থি_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = পূর্বনির্ধারিত;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        জন্য (AstCFile* ধ্রুবক cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
