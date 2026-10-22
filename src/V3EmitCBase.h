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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3File.h"

#அந்தர்பாவ <cmath>
#அந்தர்பாவ <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    மாறிலி VNUser4InUse user4InUse;

பொது:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    நிலையான மாறிலி Ast_முனை_Module* get(மாறிலி Ast_முனை* nodep) VL_MT_STABLE {
        திரும்பு VN_AS(nodep->user4p(), முனை_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
பொது:
    நிலையான string voidSelfAssign(மாறிலி Ast_முனை_Module* modp) {
        மாறிலி string className = prefixNameProtect(modp);
        திரும்பு className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    நிலையான string pchClassName() VL_MT_STABLE { திரும்பு v3Global.opt.prefix() + "__pch"; }
    நிலையான string symClassName() VL_MT_STABLE {
        திரும்பு v3Global.opt.prefix() + "_" + VIdProtect::protect("_Syms");
    }
    நிலையான string symClassVar() { திரும்பு symClassName() + "* __restrict vlSymsp"; }
    நிலையான string symClassAssign() {
        திரும்பு symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    நிலையான string உச்சி_Class_Name() VL_MT_SAFE {  // Return name of top wrapper module
        திரும்பு v3Global.opt.prefix();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    நிலையான string execGraphName(மாறிலி AstExecGraph* nodep) {
        திரும்பு உச்சி_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    நிலையான std::string constPoolName(மாறிலி AstVar* varp) {
        UASSERT_OBJ(varp->constPoolEntry(), varp, "Not a constant pool variable");
        திரும்பு உச்சி_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    நிலையான string prefixNameProtect(மாறிலி Ast_முனை* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : பொது VNVisitorConst {
பொது:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class TextStmts final {
    பொது:
        கட்டமைப்பு Stmt final {
            std::string m_text;  // Statement text
            size_t m_cost;  // Cost of statement, for splitting
        };

    தனிப்பட்ட:
        std::vector<Stmt> m_stmts;  // Statements

    பொது:
        // Add a statement
        வெறுமை add(மாறிலி std::string& stmt) {
            மாறிலி size_t cost = stmt.empty() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_முனை::INSTR_மொத்தம்_CTOR_DTOR;
            m_stmts.தள்ளு_back({stmt, cost});
        }
        size_t size() மாறிலி { திரும்பு m_stmts.size(); }
        மாறிலி Stmt& operator[](size_t i) மாறிலி { திரும்பு m_stmts[i]; }
        std::vector<Stmt>::const_iterator begin() மாறிலி { திரும்பு m_stmts.begin(); }
        std::vector<Stmt>::const_iterator end() மாறிலி { திரும்பு m_stmts.end(); }
    };

தனிப்பட்ட:
    // STATE
    V3OutCFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::vector<AstCFile*> m_newCfileps;  // AstCFiles created
    size_t m_splitSize = 0;  // Complexity of this file
    மாறிலி size_t m_splitLimit = v3Global.opt.outputSplit()
                                    ? static_cast<size_t>(v3Global.opt.outputSplit())
                                    : std::numeric_limits<size_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    வெறுமை openNewOutputFile(மாறிலி std::string& baseName, bool slow, bool support, bool source,
                           மாறிலி எழுத்து* மாறிலி descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + baseName;
        எனில் (source) {
            எனில் (slow) fileName += "__Slow";
            fileName += ".cpp";
        } இல்லையெனில் {
            fileName += ".h";
        }
        AstCFile* மாறிலி cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->source(source);
        cfilep->support(support);
        m_newCfileps.emplace_back(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emitSplitTextStmts(TextStmts& stmts, மாறிலி std::string& prefix,
                                   மாறிலி std::string& params, மாறிலி std::string& args,
                                   size_t maxCost, bool support, மாறிலி எழுத்து* descriptionp,
                                   மாறிலி std::function<வெறுமை()>& emitHeader);

பொது:
    // Create new source AstCFile and open it for writing
    வெறுமை openNewOutputSourceFile(மாறிலி std::string& baseName, bool slow, bool support,
                                 மாறிலி எழுத்து* descriptionp) {
        V3Stats::addStatSum(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(baseName, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    வெறுமை openNewOutputHeaderFile(மாறிலி std::string& baseName, மாறிலி எழுத்து* descriptionp) {
        openNewOutputFile(baseName, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    வெறுமை openOutputFile(AstCFile* cfilep, மாறிலி எழுத்து* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_splitSize = 0;
        எனில் (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3OutCFile{VL_DEV_NULL};
            திரும்பு;
        }
        m_ofp = new V3OutCFile{cfilep->name()};
        putsHeader();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    வெறுமை closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_splitSize);
        m_cfilep = nullptr;
    }

    std::vector<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        திரும்பு std::move(m_newCfileps);
    }

    வெறுமை splitSizeInc(size_t மொத்தம்) { m_splitSize += மொத்தம்; }
    வெறுமை splitSizeInc(மாறிலி Ast_முனை* nodep) {
        splitSizeInc(static_cast<size_t>(nodep->முனை_மொத்தம்()));
    }
    bool splitNeeded(size_t splitLimit) மாறிலி { திரும்பு m_splitSize >= splitLimit; }
    bool splitNeeded() மாறிலி { திரும்பு splitNeeded(m_splitLimit); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emitSplitCtorDtor(TextStmts& ctorStmts, TextStmts& dtorStmts, size_t baseCost,
                      மாறிலி std::string& prefix, மாறிலி std::string& params,
                      மாறிலி std::string& args, bool support, மாறிலி எழுத்து* descriptionp,
                      மாறிலி std::function<வெறுமை()>& emitHeader);

    // Returns pointer to current output file object.
    V3OutCFile* ofp() மாறிலி VL_MT_SAFE { திரும்பு m_ofp; }

    வெறுமை puts(மாறிலி string& str) { ofp()->puts(str); }
    வெறுமை putns(மாறிலி Ast_முனை* nodep, மாறிலி string& str) { ofp()->putns(nodep, str); }
    வெறுமை putsHeader() { ofp()->putsHeader(); }
    வெறுமை putbs(மாறிலி string& str) { ofp()->putbs(str); }
    வெறுமை putnbs(மாறிலி Ast_முனை* nodep, மாறிலி string& str) { ofp()->putnbs(nodep, str); }
    வெறுமை putsDecoration(மாறிலி Ast_முனை* nodep, மாறிலி string& str) {
        எனில் (v3Global.opt.decoration()) putns(nodep, str);
    }
    வெறுமை putsQuoted(மாறிலி string& str) { ofp()->putsQuoted(str); }
    வெறுமை ensure_New_வரி() { ofp()->ensure_New_வரி(); }
    bool optSystemC() { திரும்பு v3Global.opt.systemC(); }
    நிலையான string protect(மாறிலி string& name) VL_MT_SAFE { திரும்பு VIdProtect::protect(name); }
    நிலையான string funcNameProtect(மாறிலி AstCFunc* nodep, மாறிலி Ast_முனை_Module* modp = nullptr);
    நிலையான bool isExplicitlyConstructed(மாறிலி AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        மாறிலி Ast_முனை_Module* மாறிலி modp = EmitCParentModule::get(varp);
        எனில் (!modp || !varp->isModelState()) திரும்பு false;
        // Classes use real constructors
        எனில் (VN_IS(modp, Class)) திரும்பு false;
        // Constant pool has special handling
        எனில் (modp->isConstPool()) திரும்பு false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        திரும்பு !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    நிலையான std::string varName(மாறிலி AstVar* varp) VL_MT_STABLE {
        மாறிலி std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        எனில் (varp->isPrimaryIO()) திரும்பு "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        எனில் (isExplicitlyConstructed(varp)) திரும்பு "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        திரும்பு name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string cFuncArgs(மாறிலி AstCFunc* nodep, bool cpp = true);
    வெறுமை emitCDefaultConstructor(மாறிலி Ast_முனை_Module* modp);
    வெறுமை emitCFuncHeader(மாறிலி AstCFunc* funcp, மாறிலி Ast_முனை_Module* modp, bool withScope);
    வெறுமை emitCFuncDecl(மாறிலி AstCFunc* funcp, மாறிலி Ast_முனை_Module* modp, bool cLinkage = false);
    வெறுமை emitVarDecl(மாறிலி AstVar* nodep, bool asRef = false);
    வெறுமை emitVarAccessors(மாறிலி AstVar* nodep);
    template <typename T_Callable>
    நிலையான வெறுமை forModCUse(மாறிலி Ast_முனை_Module* modp, VUseType useType, T_Callable action) {
        க்கு (மாறிலி Ast_முனை* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            எனில் (மாறிலி AstCUse* மாறிலி usep = VN_CAST(itemp, CUse)) {
                எனில் (usep->useType().containsAny(useType)) {
                    எனில் (usep->useType().containsAny(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::prefixNameProtect(usep) + ".h\"\n");
                        தொடர்;  // Forward declaration is not necessary
                    }
                    எனில் (usep->useType().containsAny(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::prefixNameProtect(usep) + ";\n");
                    }
                }
            }
        }
    }
    வெறுமை emitModCUse(மாறிலி Ast_முனை_Module* modp, VUseType useType);
    வெறுமை emitSystemCSection(மாறிலி Ast_முனை_Module* modp, VSystemCSectionType type);
    நிலையான std::pair<string, File_வரி*> scSection(மாறிலி Ast_முனை_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = இயல்பு;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        க்கு (AstCFile* மாறிலி cfilep : m_newCfileps) v3Global.rootp()->addFilesp(cfilep);
    }
};

#endif  // guard
