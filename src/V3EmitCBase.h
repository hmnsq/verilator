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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3EMITCBASE_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3EMITCBASE_H_

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
    ಸ್ಥಿರಾಂಕ VNUser4_ಒಳ_Use user4_ಒಳ_Use;

ಸಾರ್ವಜನಿಕ:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* get(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ VN_AS(nodep->user4p(), ಗಂಟು_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ string void_Self_ನಿಯೋಜಿಸು(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಸ್ಥಿರಾಂಕ string className = ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(modp);
        ಹಿಂತಿರುಗಿ className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    ಸ್ಥಿರ string pchClassName() VL_MT_STABLE { ಹಿಂತಿರುಗಿ v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "__pch"; }
    ಸ್ಥಿರ string symClassName() VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ() + "_" + VIdProtect::protect("_Syms");
    }
    ಸ್ಥಿರ string symClassVar() { ಹಿಂತಿರುಗಿ symClassName() + "* __restrict vlSymsp"; }
    ಸ್ಥಿರ string sym_Class_ನಿಯೋಜಿಸು() {
        ಹಿಂತಿರುಗಿ symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    ಸ್ಥಿರ string ತುದಿ_Class_Name() VL_MT_ಸುರಕ್ಷಿತ {  // Return name of top wrapper module
        ಹಿಂತಿರುಗಿ v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    ಸ್ಥಿರ string exec_ನಕ್ಷೆ_Name(ಸ್ಥಿರಾಂಕ Ast_Exec_ನಕ್ಷೆ* nodep) {
        ಹಿಂತಿರುಗಿ ತುದಿ_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    ಸ್ಥಿರ std::string constPoolName(ಸ್ಥಿರಾಂಕ AstVar* varp) {
        UASSERT_OBJ(varp->const_Pool_ನಮೂದು(), varp, "Not a constant pool variable");
        ಹಿಂತಿರುಗಿ ತುದಿ_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    ಸ್ಥಿರ string ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ VNVisitorConst {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class ಪಠ್ಯ_Stmts final {
    ಸಾರ್ವಜನಿಕ:
        ರಚನೆ Stmt final {
            std::string m_ಪಠ್ಯ;  // Statement text
            ಗಾತ್ರ_t m_ವೆಚ್ಚ;  // Cost of statement, for splitting
        };

    ಖಾಸಗಿ:
        std::ಸದಿಶ<Stmt> m_stmts;  // Statements

    ಸಾರ್ವಜನಿಕ:
        // Add a statement
        ಶೂನ್ಯ ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ std::string& stmt) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ವೆಚ್ಚ = stmt.ಖಾಲಿಗಳು() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_ಗಂಟು::INSTR_ಎಣಿಕೆ_CTOR_DTOR;
            m_stmts.ತಳ್ಳು_ಹಿಂಭಾಗ({stmt, ವೆಚ್ಚ});
        }
        ಗಾತ್ರ_t ಗಾತ್ರ_2() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stmts.ಗಾತ್ರ_2(); }
        ಸ್ಥಿರಾಂಕ Stmt& ಕ್ರಿಯಾಕಾರಕ[](ಗಾತ್ರ_t i) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stmts[i]; }
        std::ಸದಿಶ<Stmt>::const_iterator begin() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stmts.begin(); }
        std::ಸದಿಶ<Stmt>::const_iterator ಕೊನೆ() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_stmts.ಕೊನೆ(); }
    };

ಖಾಸಗಿ:
    // STATE
    V3_ಹೊರಹೋಗು_CFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::ಸದಿಶ<AstCFile*> m_newCfileps;  // AstCFiles created
    ಗಾತ್ರ_t m_ವಿಭಜಿಸು_ಗಾತ್ರ = 0;  // Complexity of this file
    ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t m_ವಿಭಜಿಸು_ಎಲ್ಲೆ = v3Global.opt.output_ವಿಭಜಿಸು()
                                    ? static_cast<ಗಾತ್ರ_t>(v3Global.opt.output_ವಿಭಜಿಸು())
                                    : std::numeric_limits<ಗಾತ್ರ_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    ಶೂನ್ಯ openNewOutputFile(ಸ್ಥಿರಾಂಕ std::string& ತಳ_Name, bool slow, bool support, bool ಮೂಲ,
                           ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* ಸ್ಥಿರಾಂಕ descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + ತಳ_Name;
        ಆದರೆ (ಮೂಲ) {
            ಆದರೆ (slow) fileName += "__Slow";
            fileName += ".cpp";
        } ಇಲ್ಲದಿದ್ದರೆ {
            fileName += ".h";
        }
        AstCFile* ಸ್ಥಿರಾಂಕ cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->ಮೂಲ(ಮೂಲ);
        cfilep->support(support);
        m_newCfileps.emplace_ಹಿಂಭಾಗ(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emit_ವಿಭಜಿಸು_ಪಠ್ಯ_Stmts(ಪಠ್ಯ_Stmts& stmts, ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ,
                                   ಸ್ಥಿರಾಂಕ std::string& params, ಸ್ಥಿರಾಂಕ std::string& ವಾದಗಳು,
                                   ಗಾತ್ರ_t max_ವೆಚ್ಚ, bool support, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descriptionp,
                                   ಸ್ಥಿರಾಂಕ std::function<ಶೂನ್ಯ()>& emit_ಶೀರ್ಷಿಕೆ);

ಸಾರ್ವಜನಿಕ:
    // Create new source AstCFile and open it for writing
    ಶೂನ್ಯ open_New_Output_ಮೂಲ_File(ಸ್ಥಿರಾಂಕ std::string& ತಳ_Name, bool slow, bool support,
                                 ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descriptionp) {
        V3Stats::ಕೂಡಿಸು_Stat_ಮೊತ್ತ(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(ತಳ_Name, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    ಶೂನ್ಯ open_New_Output_ಶೀರ್ಷಿಕೆ_File(ಸ್ಥಿರಾಂಕ std::string& ತಳ_Name, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descriptionp) {
        openNewOutputFile(ತಳ_Name, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    ಶೂನ್ಯ openOutputFile(AstCFile* cfilep, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_ವಿಭಜಿಸು_ಗಾತ್ರ = 0;
        ಆದರೆ (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3_ಹೊರಹೋಗು_CFile{VL_DEV_NULL};
            ಹಿಂತಿರುಗಿ;
        }
        m_ofp = new V3_ಹೊರಹೋಗು_CFile{cfilep->name()};
        puts_ಶೀರ್ಷಿಕೆ();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    ಶೂನ್ಯ closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_ವಿಭಜಿಸು_ಗಾತ್ರ);
        m_cfilep = nullptr;
    }

    std::ಸದಿಶ<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        ಹಿಂತಿರುಗಿ std::move(m_newCfileps);
    }

    ಶೂನ್ಯ ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(ಗಾತ್ರ_t ಎಣಿಕೆ_2) { m_ವಿಭಜಿಸು_ಗಾತ್ರ += ಎಣಿಕೆ_2; }
    ಶೂನ್ಯ ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep) {
        ವಿಭಜಿಸು_ಗಾತ್ರ_Inc(static_cast<ಗಾತ್ರ_t>(nodep->ಗಂಟು_ಎಣಿಕೆ()));
    }
    bool ವಿಭಜಿಸು_Needed(ಗಾತ್ರ_t ವಿಭಜಿಸು_ಎಲ್ಲೆ) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_ವಿಭಜಿಸು_ಗಾತ್ರ >= ವಿಭಜಿಸು_ಎಲ್ಲೆ; }
    bool ವಿಭಜಿಸು_Needed() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ ವಿಭಜಿಸು_Needed(m_ವಿಭಜಿಸು_ಎಲ್ಲೆ); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emit_ವಿಭಜಿಸು_Ctor_Dtor(ಪಠ್ಯ_Stmts& ctorStmts, ಪಠ್ಯ_Stmts& dtorStmts, ಗಾತ್ರ_t ತಳ_ವೆಚ್ಚ,
                      ಸ್ಥಿರಾಂಕ std::string& ಪೂರ್ವಪ್ರತ್ಯಯ, ಸ್ಥಿರಾಂಕ std::string& params,
                      ಸ್ಥಿರಾಂಕ std::string& ವಾದಗಳು, bool support, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* descriptionp,
                      ಸ್ಥಿರಾಂಕ std::function<ಶೂನ್ಯ()>& emit_ಶೀರ್ಷಿಕೆ);

    // Returns pointer to current output file object.
    V3_ಹೊರಹೋಗು_CFile* ofp() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ m_ofp; }

    ಶೂನ್ಯ puts(ಸ್ಥಿರಾಂಕ string& str) { ofp()->puts(str); }
    ಶೂನ್ಯ putns(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& str) { ofp()->putns(nodep, str); }
    ಶೂನ್ಯ puts_ಶೀರ್ಷಿಕೆ() { ofp()->puts_ಶೀರ್ಷಿಕೆ(); }
    ಶೂನ್ಯ putbs(ಸ್ಥಿರಾಂಕ string& str) { ofp()->putbs(str); }
    ಶೂನ್ಯ putnbs(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& str) { ofp()->putnbs(nodep, str); }
    ಶೂನ್ಯ putsDecoration(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* nodep, ಸ್ಥಿರಾಂಕ string& str) {
        ಆದರೆ (v3Global.opt.decoration()) putns(nodep, str);
    }
    ಶೂನ್ಯ putsQuoted(ಸ್ಥಿರಾಂಕ string& str) { ofp()->putsQuoted(str); }
    ಶೂನ್ಯ ensure_New_ಸಾಲು() { ofp()->ensure_New_ಸಾಲು(); }
    bool optSystemC() { ಹಿಂತಿರುಗಿ v3Global.opt.systemC(); }
    ಸ್ಥಿರ string protect(ಸ್ಥಿರಾಂಕ string& name) VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ VIdProtect::protect(name); }
    ಸ್ಥಿರ string funcNameProtect(ಸ್ಥಿರಾಂಕ AstCFunc* nodep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp = nullptr);
    ಸ್ಥಿರ bool isExplicitlyConstructed(ಸ್ಥಿರಾಂಕ AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* ಸ್ಥಿರಾಂಕ modp = EmitCParentModule::get(varp);
        ಆದರೆ (!modp || !varp->isModelState()) ಹಿಂತಿರುಗಿ false;
        // Classes use real constructors
        ಆದರೆ (VN_IS(modp, Class)) ಹಿಂತಿರುಗಿ false;
        // Constant pool has special handling
        ಆದರೆ (modp->isConstPool()) ಹಿಂತಿರುಗಿ false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        ಹಿಂತಿರುಗಿ !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    ಸ್ಥಿರ std::string varName(ಸ್ಥಿರಾಂಕ AstVar* varp) VL_MT_STABLE {
        ಸ್ಥಿರಾಂಕ std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        ಆದರೆ (varp->isPrimaryIO()) ಹಿಂತಿರುಗಿ "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        ಆದರೆ (isExplicitlyConstructed(varp)) ಹಿಂತಿರುಗಿ "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        ಹಿಂತಿರುಗಿ name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string c_Func_ವಾದಗಳು(ಸ್ಥಿರಾಂಕ AstCFunc* nodep, bool cpp = true);
    ಶೂನ್ಯ emitCDefaultConstructor(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp);
    ಶೂನ್ಯ emit_CFunc_ಶೀರ್ಷಿಕೆ(ಸ್ಥಿರಾಂಕ AstCFunc* funcp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, bool withScope);
    ಶೂನ್ಯ emitCFuncDecl(ಸ್ಥಿರಾಂಕ AstCFunc* funcp, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, bool cLinkage = false);
    ಶೂನ್ಯ emitVarDecl(ಸ್ಥಿರಾಂಕ AstVar* nodep, bool asRef = false);
    ಶೂನ್ಯ emitVarAccessors(ಸ್ಥಿರಾಂಕ AstVar* nodep);
    ಮಾದರಿ <typename T_Callable>
    ಸ್ಥಿರ ಶೂನ್ಯ forModCUse(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, VUseType useType, T_Callable action) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstCUse* ಸ್ಥಿರಾಂಕ usep = VN_CAST(itemp, CUse)) {
                ಆದರೆ (usep->useType().ಒಳಗೊಂಡಿದೆ_Any(useType)) {
                    ಆದರೆ (usep->useType().ಒಳಗೊಂಡಿದೆ_Any(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(usep) + ".h\"\n");
                        ಮುಂದುವರಿ;  // Forward declaration is not necessary
                    }
                    ಆದರೆ (usep->useType().ಒಳಗೊಂಡಿದೆ_Any(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::ಪೂರ್ವಪ್ರತ್ಯಯ_Name_Protect(usep) + ";\n");
                    }
                }
            }
        }
    }
    ಶೂನ್ಯ emitModCUse(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, VUseType useType);
    ಶೂನ್ಯ emitSystemCSection(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp, VSystemCSectionType type);
    ಸ್ಥಿರ std::pair<string, File_ಸಾಲು*> scSection(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = ಪೂರ್ವನಿಯೋಜಿತ;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        ಗಾಗಿ (AstCFile* ಸ್ಥಿರಾಂಕ cfilep : m_newCfileps) v3Global.rootp()->ಕೂಡಿಸು_Filesp(cfilep);
    }
};

#ಆದರೆ_ಅಂತ್ಯ  // guard
