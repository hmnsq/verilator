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

#నిర్వచితం_కాకపోతే VERILATOR_V3EMITCBASE_H_
#నిర్వచించు VERILATOR_V3EMITCBASE_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3File.h"

#అన్తర్భావ <cmath>
#అన్తర్భావ <cstdarg>

//######################################################################
// Set user4p in all CFunc, Var, and coverage declarations to point to the
// containing AstNodeModule

class EmitCParentModule final {
    // NODE STATE
    //   AstCFunc::user4p()          AstNodeModule* Parent module pointer
    //   AstVar::user4p()            AstNodeModule* Parent module pointer
    //   AstNodeCoverDecl::user4p()  AstNodeModule* Parent module pointer
    స్థిరరాశి VNUser4_లోపల_Use user4_లోపల_Use;

బహిరంగ:
    EmitCParentModule();
    VL_UNCOPYABLE(EmitCParentModule);

    స్థిర స్థిరరాశి Ast_కణుపు_Module* get(స్థిరరాశి Ast_కణుపు* nodep) VL_MT_STABLE {
        తిరిగి VN_AS(nodep->user4p(), కణుపు_Module);
    }
};

//######################################################################
// EmitC-related utility functions

class EmitCUtil final {
బహిరంగ:
    స్థిర string void_Self_కేటాయించు(స్థిరరాశి Ast_కణుపు_Module* modp) {
        స్థిరరాశి string className = ఉపసర్గ_Name_Protect(modp);
        తిరిగి className + "* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<" + className
               + "*>(voidSelf);\n";
    }
    స్థిర string pchClassName() VL_MT_STABLE { తిరిగి v3Global.opt.ఉపసర్గ() + "__pch"; }
    స్థిర string symClassName() VL_MT_STABLE {
        తిరిగి v3Global.opt.ఉపసర్గ() + "_" + VIdProtect::protect("_Syms");
    }
    స్థిర string symClassVar() { తిరిగి symClassName() + "* __restrict vlSymsp"; }
    స్థిర string sym_Class_కేటాయించు() {
        తిరిగి symClassName() + "* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;\n";
    }
    స్థిర string శిఖరం_Class_Name() VL_MT_సురక్షిత {  // Return name of top wrapper module
        తిరిగి v3Global.opt.ఉపసర్గ();
    }
    // Return name of the VlExecGraph describing an AstExecGraph
    స్థిర string exec_లేఖాచిత్రం_Name(స్థిరరాశి Ast_Exec_లేఖాచిత్రం* nodep) {
        తిరిగి శిఖరం_Class_Name() + "__ExecGraph__" + nodep->nameProtect();
    }
    // Name of a constant pool variable
    స్థిర std::string constPoolName(స్థిరరాశి AstVar* varp) {
        UASSERT_OBJ(varp->const_Pool_నమోదు(), varp, "Not a constant pool variable");
        తిరిగి శిఖరం_Class_Name() + "__ConstPool__" + varp->nameProtect();
    }
    // Return C++ class name for a module/class object
    స్థిర string ఉపసర్గ_Name_Protect(స్థిరరాశి Ast_కణుపు* nodep) VL_MT_STABLE;
};

//######################################################################
// Base Visitor class -- holds output file pointer

class EmitCBaseVisitorConst VL_NOT_FINAL : బహిరంగ VNVisitorConst {
బహిరంగ:
    // TYPES
    // Rendered text statements, each with its cost, for splitting them into sub-functions
    class పాఠ్యం_Stmts final {
    బహిరంగ:
        నిర్మాణం Stmt final {
            std::string m_పాఠ్యం;  // Statement text
            కొలత_t m_ఖర్చు;  // Cost of statement, for splitting
        };

    అంతరంగ:
        std::సదిశ<Stmt> m_stmts;  // Statements

    బహిరంగ:
        // Add a statement
        శూన్య కూడు(స్థిరరాశి std::string& stmt) {
            స్థిరరాశి కొలత_t ఖర్చు = stmt.ఖాళీలు() || VString::startsWith(stmt, "//")
                                    ? 0  // Blanks and comments are free
                                    : Ast_కణుపు::INSTR_లెక్క_CTOR_DTOR;
            m_stmts.నెట్టు_వెనుక({stmt, ఖర్చు});
        }
        కొలత_t కొలత() స్థిరరాశి { తిరిగి m_stmts.కొలత(); }
        స్థిరరాశి Stmt& క్రియాకారకం[](కొలత_t i) స్థిరరాశి { తిరిగి m_stmts[i]; }
        std::సదిశ<Stmt>::const_iterator begin() స్థిరరాశి { తిరిగి m_stmts.begin(); }
        std::సదిశ<Stmt>::const_iterator చివర() స్థిరరాశి { తిరిగి m_stmts.చివర(); }
    };

అంతరంగ:
    // STATE
    V3_బహిర్గతం_CFile* m_ofp = nullptr;  // File handle to emit to
    AstCFile* m_cfilep = nullptr;  // Current AstCFile being emitted
    std::సదిశ<AstCFile*> m_newCfileps;  // AstCFiles created
    కొలత_t m_విభజించు_కొలత = 0;  // Complexity of this file
    స్థిరరాశి కొలత_t m_విభజించు_హద్దు = v3Global.opt.output_విభజించు()
                                    ? static_cast<కొలత_t>(v3Global.opt.output_విభజించు())
                                    : std::numeric_limits<కొలత_t>::max();

    // METHODS

    // Create new AstCFile and open it for writing
    శూన్య openNewOutputFile(స్థిరరాశి std::string& పునాది_Name, bool slow, bool support, bool ఆధారం,
                           స్థిరరాశి అక్షరం* స్థిరరాశి descriptionp) {
        std::string fileName = v3Global.opt.makeDir() + "/" + పునాది_Name;
        ఒకవేళ (ఆధారం) {
            ఒకవేళ (slow) fileName += "__Slow";
            fileName += ".cpp";
        } లేకపోతే {
            fileName += ".h";
        }
        AstCFile* స్థిరరాశి cfilep = new AstCFile{v3Global.rootp()->fileline(), fileName};
        cfilep->slow(slow);
        cfilep->ఆధారం(ఆధారం);
        cfilep->support(support);
        m_newCfileps.emplace_వెనుక(cfilep);
        openOutputFile(cfilep, descriptionp);
    }

    // Reduce 'stmts' to a single statement, via a balanced tree of loose sub-functions
    // 'void <prefix>__<n>(<params>)' of at most 'maxCost' each (see emitSplitCtorDtor).
    // Returns the declaration of the root sub-function, or empty if nothing was split.
    std::string emit_విభజించు_పాఠ్యం_Stmts(పాఠ్యం_Stmts& stmts, స్థిరరాశి std::string& ఉపసర్గ,
                                   స్థిరరాశి std::string& params, స్థిరరాశి std::string& వాదనలు,
                                   కొలత_t max_ఖర్చు, bool support, స్థిరరాశి అక్షరం* descriptionp,
                                   స్థిరరాశి std::function<శూన్య()>& emit_శీర్షిక);

బహిరంగ:
    // Create new source AstCFile and open it for writing
    శూన్య open_New_Output_ఆధారం_File(స్థిరరాశి std::string& పునాది_Name, bool slow, bool support,
                                 స్థిరరాశి అక్షరం* descriptionp) {
        V3Stats::కూడు_Stat_కూడిక(V3Stats::STAT_CPP_FILES, 1);
        openNewOutputFile(పునాది_Name, slow, support, true, descriptionp);
    }

    // Create new header AstCFile and open it for writing
    శూన్య open_New_Output_శీర్షిక_File(స్థిరరాశి std::string& పునాది_Name, స్థిరరాశి అక్షరం* descriptionp) {
        openNewOutputFile(పునాది_Name, false, false, false, descriptionp);
    }

    // Open exisitng AstCFile for writing
    శూన్య openOutputFile(AstCFile* cfilep, స్థిరరాశి అక్షరం* descriptionp) {
        UASSERT(!m_ofp, "Output file is already open");
        m_cfilep = cfilep;
        m_విభజించు_కొలత = 0;
        ఒకవేళ (v3Global.opt.lintOnly()) {
            // Unfortunately we have some lint checks in EmitCImp, so we can't
            // just skip processing. TODO: Move them to an earlier stage.
            m_ofp = new V3_బహిర్గతం_CFile{VL_DEV_NULL};
            తిరిగి;
        }
        m_ofp = new V3_బహిర్గతం_CFile{cfilep->name()};
        puts_శీర్షిక();
        // Emit description
        m_ofp->putsNoTracking("// DESCR" /* keep this comment */ "IPTION: Verilator output: ");
        m_ofp->putsNoTracking(descriptionp);
        m_ofp->putsNoTracking("\n");
    }

    // Close current output file. Sets ofp() and outFileNodep() to nullptr.
    శూన్య closeOutputFile() {
        UASSERT(m_ofp, "No currently open output file");
        VL_DO_CLEAR(delete m_ofp, m_ofp = nullptr);
        m_cfilep->complexityScore(m_విభజించు_కొలత);
        m_cfilep = nullptr;
    }

    std::సదిశ<AstCFile*> getAndClearCfileps() {
        UASSERT(!m_ofp, "Output file is open");
        తిరిగి std::move(m_newCfileps);
    }

    శూన్య విభజించు_కొలత_Inc(కొలత_t లెక్క) { m_విభజించు_కొలత += లెక్క; }
    శూన్య విభజించు_కొలత_Inc(స్థిరరాశి Ast_కణుపు* nodep) {
        విభజించు_కొలత_Inc(static_cast<కొలత_t>(nodep->కణుపు_లెక్క()));
    }
    bool విభజించు_Needed(కొలత_t విభజించు_హద్దు) స్థిరరాశి { తిరిగి m_విభజించు_కొలత >= విభజించు_హద్దు; }
    bool విభజించు_Needed() స్థిరరాశి { తిరిగి విభజించు_Needed(m_విభజించు_హద్దు); }

    // If the given constructor/destructor statements, plus 'baseCost', exceed the
    // --output-split-cfuncs limit, split them into loose sub-functions
    // 'void <prefix>__ctor__<n>(<params>)' and 'void <prefix>__dtor__<n>(<params>)', called
    // with '<args>'. Each is emitted into its own file, starting with 'emitHeader'. The
    // statements are then each replaced with a single call to the root sub-function. Returns
    // the declarations of the root ctor/dtor sub-functions, or empty strings if not split.
    std::pair<std::string, std::string>
    emit_విభజించు_Ctor_Dtor(పాఠ్యం_Stmts& ctorStmts, పాఠ్యం_Stmts& dtorStmts, కొలత_t పునాది_ఖర్చు,
                      స్థిరరాశి std::string& ఉపసర్గ, స్థిరరాశి std::string& params,
                      స్థిరరాశి std::string& వాదనలు, bool support, స్థిరరాశి అక్షరం* descriptionp,
                      స్థిరరాశి std::function<శూన్య()>& emit_శీర్షిక);

    // Returns pointer to current output file object.
    V3_బహిర్గతం_CFile* ofp() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_ofp; }

    శూన్య puts(స్థిరరాశి string& str) { ofp()->puts(str); }
    శూన్య putns(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& str) { ofp()->putns(nodep, str); }
    శూన్య puts_శీర్షిక() { ofp()->puts_శీర్షిక(); }
    శూన్య putbs(స్థిరరాశి string& str) { ofp()->putbs(str); }
    శూన్య putnbs(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& str) { ofp()->putnbs(nodep, str); }
    శూన్య putsDecoration(స్థిరరాశి Ast_కణుపు* nodep, స్థిరరాశి string& str) {
        ఒకవేళ (v3Global.opt.decoration()) putns(nodep, str);
    }
    శూన్య putsQuoted(స్థిరరాశి string& str) { ofp()->putsQuoted(str); }
    శూన్య ensure_New_పంక్తి() { ofp()->ensure_New_పంక్తి(); }
    bool optSystemC() { తిరిగి v3Global.opt.systemC(); }
    స్థిర string protect(స్థిరరాశి string& name) VL_MT_సురక్షిత { తిరిగి VIdProtect::protect(name); }
    స్థిర string funcNameProtect(స్థిరరాశి AstCFunc* nodep, స్థిరరాశి Ast_కణుపు_Module* modp = nullptr);
    స్థిర bool isExplicitlyConstructed(స్థిరరాశి AstVar* varp) VL_MT_STABLE {
        // Only model member variables may be handled explicitly
        స్థిరరాశి Ast_కణుపు_Module* స్థిరరాశి modp = EmitCParentModule::get(varp);
        ఒకవేళ (!modp || !varp->isModelState()) తిరిగి false;
        // Classes use real constructors
        ఒకవేళ (VN_IS(modp, Class)) తిరిగి false;
        // Constant pool has special handling
        ఒకవేళ (modp->isConstPool()) తిరిగి false;
        // Otherwise explicitly handled if not trivially constructible,
        // meaning a runtime C++ constructor/destructor invocation is needed
        తిరిగి !varp->isTriviallyConstructible();
    }
    // Name to access a module member as
    స్థిర std::string varName(స్థిరరాశి AstVar* varp) VL_MT_STABLE {
        స్థిరరాశి std::string name = varp->nameProtect();
        // Primary IOs live 'union { struct { HERE } __VprimaryPorts; }'
        ఒకవేళ (varp->isPrimaryIO()) తిరిగి "__VprimaryPorts." + name;
        // Explicitly constructed variables live 'union { struct { HERE } __Vnontrivs; }'
        ఒకవేళ (isExplicitlyConstructed(varp)) తిరిగి "__Vnontrivs." + name;
        // Otherwise they should be available in the local scope where emitted
        తిరిగి name;
    }
    // Argument list; if 'cpp', C++ rather than a C header
    string c_Func_వాదనలు(స్థిరరాశి AstCFunc* nodep, bool cpp = true);
    శూన్య emitCDefaultConstructor(స్థిరరాశి Ast_కణుపు_Module* modp);
    శూన్య emit_CFunc_శీర్షిక(స్థిరరాశి AstCFunc* funcp, స్థిరరాశి Ast_కణుపు_Module* modp, bool withScope);
    శూన్య emitCFuncDecl(స్థిరరాశి AstCFunc* funcp, స్థిరరాశి Ast_కణుపు_Module* modp, bool cLinkage = false);
    శూన్య emitVarDecl(స్థిరరాశి AstVar* nodep, bool asRef = false);
    శూన్య emitVarAccessors(స్థిరరాశి AstVar* nodep);
    మూస <typename T_Callable>
    స్థిర శూన్య forModCUse(స్థిరరాశి Ast_కణుపు_Module* modp, VUseType useType, T_Callable action) {
        కొరకు (స్థిరరాశి Ast_కణుపు* itemp = modp->stmtsp(); itemp; itemp = itemp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstCUse* స్థిరరాశి usep = VN_CAST(itemp, CUse)) {
                ఒకవేళ (usep->useType().కలిగియుంది_Any(useType)) {
                    ఒకవేళ (usep->useType().కలిగియుంది_Any(VUseType::INT_INCLUDE)) {
                        action("#include \"" + EmitCUtil::ఉపసర్గ_Name_Protect(usep) + ".h\"\n");
                        కొనసాగు;  // Forward declaration is not necessary
                    }
                    ఒకవేళ (usep->useType().కలిగియుంది_Any(VUseType::INT_FWD_CLASS)) {
                        action("class " + EmitCUtil::ఉపసర్గ_Name_Protect(usep) + ";\n");
                    }
                }
            }
        }
    }
    శూన్య emitModCUse(స్థిరరాశి Ast_కణుపు_Module* modp, VUseType useType);
    శూన్య emitSystemCSection(స్థిరరాశి Ast_కణుపు_Module* modp, VSystemCSectionType type);
    స్థిర std::pair<string, File_పంక్తి*> scSection(స్థిరరాశి Ast_కణుపు_Module* modp,
                                                  VSystemCSectionType type);

    // CONSTRUCTORS
    EmitCBaseVisitorConst() = అప్రమేయ;
    ~EmitCBaseVisitorConst() override {
        UASSERT(!m_ofp, "Did not close output file");
        // Add files cerated to the netlist (unless retrieved before destruction)
        కొరకు (AstCFile* స్థిరరాశి cfilep : m_newCfileps) v3Global.rootp()->కూడు_Filesp(cfilep);
    }
};

#అయితే_ముగింపు  // guard
